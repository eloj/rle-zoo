OPT=-O3 -fomit-frame-pointer -funroll-loops -fstrict-aliasing -march=native -mtune=native -msse4.2 -mavx
WARNFLAGS=-Wall -Wextra -Wshadow -Wstrict-aliasing -Wcast-qual -Wcast-align -Wpointer-arith -Wredundant-decls -Wfloat-equal -Wswitch-enum
CWARNFLAGS=-Wstrict-prototypes -Wmissing-prototypes
MISCFLAGS=-fstack-protector -fvisibility=hidden
DEVFLAGS=-ggdb -DDEBUG -Wno-unused
STRICT_FLAGS=-Werror -Wconversion

RLE_VARIANTS:=goldbox packbits pcx
RLE_VARIANT_HEADERS:=$(addprefix rle_, $(RLE_VARIANTS:=.h))
RLE_VARIANT_OPS_HEADERS:=$(addprefix ops-, $(RLE_VARIANTS:=.h))

AFLCC?=afl-clang-fast

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

ifdef MEMCHECK
	TEST_PREFIX:=valgrind --tool=memcheck --leak-check=full --track-origins=yes
endif

ifdef PERF
	TEST_PREFIX:=perf stat
endif

ifdef OPTIMIZED
	MISCFLAGS+=-DNDEBUG -Werror
else
	MISCFLAGS+=$(DEVFLAGS)
endif

CFLAGS=-std=c11 $(OPT) $(CWARNFLAGS) $(WARNFLAGS) $(MISCFLAGS)

.PHONY: clean backup fuzz

all: tools tests

tools: rle-zoo rle-genops rle-parser

tests: test_rle test_utility

rle-zoo: rle-zoo.c $(RLE_VARIANT_HEADERS)
	$(CC) $(CFLAGS) $< $(filter %.o, $^) -o $@

rle-genops: rle-genops.c
	$(CC) $(CFLAGS) $< $(filter %.o, $^) -o $@

rle-parser: rle-parser.c $(RLE_VARIANT_OPS_HEADERS) utility.h
	$(CC) $(CFLAGS) $< $(filter %.o, $^) -o $@

test_rle: test_rle.c $(RLE_VARIANT_HEADERS) utility.h
	$(CC) $(CFLAGS) $< $(filter %.o, $^) -o $@

test_utility: test_utility.c utility.h
	$(CC) $(CFLAGS) $< $(filter %.o, $^) -o $@

test_example: test_example.c rle_packbits.h
	$(CC) $(CFLAGS) $< $(filter %.o, $^) -o $@

test_includeall: test_includeall.c $(RLE_VARIANT_HEADERS)
	$(CC) $(CFLAGS) $(STRICT_FLAGS) test_includeall.c -o $@

test: test_rle test_utility test_example
	$(TEST_PREFIX) ./test_utility
	$(TEST_PREFIX) ./test_rle

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

ops-%.h: rle-genops
	./rle-genops --genc $* >$@

afl-%: fuzzing/afl_*.c $(RLE_VARIANT_HEADERS)
	$(AFLCC) $(CFLAGS) -I. fuzzing/afl_$(subst -,_,$*).c -o $@

fuzz-%:
	make afl-$*
	AFL_AUTORESUME=1 AFL_SKIP_CPUFREQ=1 afl-fuzz -m 16 -i tests -o findings -- ./afl-$*

fuzz: fuzz-driver

backup:
	@echo -e $(YELLOW)Making backup$(NC)
	tar -cJf ../$(notdir $(CURDIR))-`date +"%Y-%m"`.tar.xz ../$(notdir $(CURDIR))

clean:
	@echo -e $(YELLOW)Cleaning$(NC)
	rm -f rle-zoo rle-genops rle-parser test_rle test_utility test_example test_includeall afl-driver $(RLE_VARIANT_OPS_HEADERS) vgcore.* core.* *.gcda
