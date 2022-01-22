OPT=-O3 -fomit-frame-pointer -funroll-loops -fstrict-aliasing -march=native -mtune=native -msse4.2 -mavx
WARNFLAGS=-Wall -Wextra -Wshadow -Wstrict-aliasing -Wcast-qual -Wcast-align -Wpointer-arith -Wredundant-decls -Wfloat-equal -Wswitch-enum
CWARNFLAGS=-Wstrict-prototypes -Wmissing-prototypes
MISCFLAGS=-fstack-protector -fvisibility=hidden
DEVFLAGS=-ggdb -DDEBUG -Wno-unused

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

.PHONY: clean backup

all: test_rle rle-zoo rle-genops

rle-zoo: rle-zoo.c rle_goldbox.h
	$(CC) $(CFLAGS) $< $(filter %.o, $^) -o $@

rle-genops: rle-genops.c
	$(CC) $(CFLAGS) $< $(filter %.o, $^) -o $@

test_rle: test_rle.c rle_goldbox.h
	$(CC) $(CFLAGS) $< $(filter %.o, $^) -o $@

test: all
	$(TEST_PREFIX) ./test_rle

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

backup:
	@echo -e $(YELLOW)Making backup$(NC)
	tar -cJf ../$(notdir $(CURDIR))-`date +"%Y-%m"`.tar.xz ../$(notdir $(CURDIR))

clean:
	@echo -e $(YELLOW)Cleaning$(NC)
	rm -f rle-zoo rle-genops test_rle vgcore.* core.* *.gcda
