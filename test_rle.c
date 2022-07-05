/*
	RLE Zoo Encode & Decode Tests
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	See https://github.com/eloj/rle-zoo
*/
#define _GNU_SOURCE

#define UTILITY_IMPLEMENTATION
#include "utility.h"

#define RLE_ZOO_IMPLEMENTATION
#include "rle_goldbox.h"
#include "rle_packbits.h"
#include "rle_pcx.h"
#include "rle_icns.h"

#include "rle-variant-selection.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <sys/mman.h>

#define RED "\e[1;31m"
#define GREEN "\e[0;32m"
#define YELLOW "\e[1;33m"
#define NC "\e[0m"

static int debug = 1; // Output debug hex dumps for failed tests.
static int hex_always = 0;
static int hex_show_offset = 1;
static int flag_roundtrip = 1;

static int num_roundtrip = 0;


struct test {
	uint8_t *input;
	size_t len;
	char *actions;
	ssize_t expected_size;
	uint32_t expected_hash;	// CRC32c for now
};


/*
	TODO: Should use some other digest with a simple plain-c implementation.
*/
__attribute__ ((target ("sse4.2")))
static uint32_t crc32c(uint32_t crc, const void *data, size_t len) {
	const uint8_t *src = data;

	for (size_t i=0 ; i < len ; ++i) {
		crc = __builtin_ia32_crc32qi(crc, *src++);
	}

	return crc;
}


#define TEST_ERRMSG(fmt, ...) \
	fprintf(stderr, "%s:%zu:" RED " error: " NC fmt "\n", filename, line_no __VA_OPT__(,) __VA_ARGS__)
#define TEST_WARNMSG(fmt, ...) \
	fprintf(stderr, "%s:%zu:" YELLOW " warning: " NC fmt "\n", filename, line_no __VA_OPT__(,) __VA_ARGS__)

// This either compress or decompress the output of a test to check it against the original input.
static int roundtrip(struct rle_t *rle, struct test *te, uint8_t *inbuf, size_t inbuf_len, int compress) {
	rle_fp rle_func = compress ? rle->compress : rle->decompress;

	uint8_t *tmp_buf = malloc(te->len);

	ssize_t res = rle_func(inbuf, inbuf_len, tmp_buf, te->len);

	int cmp = memcmp(tmp_buf, te->input, te->len);
	if (cmp != 0) {
		printf("expected from %scompressed test input:\n", compress ? "" : "de");
		fprint_hex(stdout, te->input, te->len, 32, "\n", hex_show_offset);
		fflush(stdout);
		printf("\n");
		printf("got");
		if (res <= 0) {
			fprintf(stdout, " error: %zd -- buffer length unknown!\n", res);
			fprint_hex(stdout, tmp_buf, te->len, 32, "\n", hex_show_offset);
			fprintf(stdout, " ...<truncated>");
		} else {
			fprintf(stdout, " %zu bytes:\n", res);
			fprint_hex(stdout, tmp_buf, res, 32, "\n", hex_show_offset);
		}
		fflush(stdout);
		printf("\n");
	} else {
		cmp = res != (ssize_t)te->len;
	}

	free(tmp_buf);

	++num_roundtrip;

	return cmp;
}

static int run_rle_test(struct rle_t *rle, struct test *te, const char *filename, size_t line_no) {
	// Take the max of the input and expected sizes as base estimate for temporary buffer.
	size_t tmp_size = te->len;
	if (te->expected_size > (ssize_t)tmp_size)
		tmp_size = te->expected_size;
	tmp_size *= 4;
	assert(tmp_size < 1L << 24);
	uint8_t *tmp_buf = malloc(tmp_size);

	int retval = 0;

	char *action = te->actions;
	int no_roundtrip = strchr(action, '-') != NULL;
	ssize_t res = 0;

	if (*action == 'c') {
		// COMPRESS

		// First do a length-determination check on the input.
		ssize_t len_check = rle->compress(te->input, te->len, NULL, 0);
		if (len_check != te->expected_size) {
			TEST_ERRMSG("expected compressed size %zu, got %zd.", te->expected_size, len_check);
			retval = 1;
		}
		if (len_check >= 0) {
			// Next compress the input into the oversized buffer, and verify length remains the same.
			assert(len_check <= (ssize_t)tmp_size);
			res = rle->compress(te->input, te->len, tmp_buf, tmp_size);
			if (res != len_check) {
				TEST_ERRMSG("compressed output length differs from determined value %zd, got %zd.", len_check, res);
				retval = 1;
			}

			uint32_t res_hash = crc32c((uint32_t)~0, tmp_buf, res) ^ (uint32_t)~0;

			// Now decompress with the output byte-tight, to check for dest range-check errors.
			ssize_t res_tight = rle->compress(te->input, te->len, tmp_buf, len_check);
			if (res_tight != len_check) {
				TEST_ERRMSG("compressed output length for tight buffer differs from determined value %zd, got %zd.", len_check, res_tight);
				retval = 1;
			}

			uint32_t res_tight_hash = crc32c((uint32_t)~0, tmp_buf, res_tight) ^ (uint32_t)~0;
			if (res_hash != te->expected_hash) {
				TEST_ERRMSG("expected compressed hash 0x%08x, got 0x%08x.", te->expected_hash, res_hash);
				retval = 1;
			}

			// Verify there's no content diff between the oversized output buffer and the tight one.
			if (res_tight_hash != res_hash) {
				TEST_ERRMSG("compressed hash mismatch; 0x%08x vs 0x%08x.", res_tight_hash, res_hash);
				retval = 1;
			}

			if (flag_roundtrip && !no_roundtrip && roundtrip(rle, te, tmp_buf, te->expected_size, 0) != 0) {
				TEST_ERRMSG("re-decompressed data does not match original input!");
				retval = 1;
			}

			if ((debug && retval != 0) || hex_always) {
				if (res < 0) {
					fprintf(stdout, "error: %zd -- hexdump unavailable, buffer length unknown!", res);
				} else {
					fprint_hex(stdout, tmp_buf, res, 32, "\n", hex_show_offset);
				}
				printf("\n");
				fflush(stdout);
			}
		} else {
			// end length check/input validation
		}
	} else if (*action == 'd') {
		// DECOMPRESS

		// First do a length-determination check on the input.
		ssize_t len_check = rle->decompress(te->input, te->len, NULL, 0);
		if (len_check != te->expected_size) {
			TEST_ERRMSG("expected decompressed size %zd, got %zd.", te->expected_size, len_check);
			retval = 1;
		}
		if (len_check > 0) {
			// Next decompress the input into the oversized buffer, and verify length remains the same.
			assert(len_check <= (ssize_t)tmp_size);
			res = rle->decompress(te->input, te->len, tmp_buf, tmp_size);
			if (res != len_check) {
				TEST_ERRMSG("decompressed output length differs from determined value %zd, got %zd.", len_check, res);
				retval = 1;
			}

			uint32_t res_hash = crc32c((uint32_t)~0, tmp_buf, res) ^ (uint32_t)~0;

			// Now decompress with the output byte-tight, to check for dest range-check errors.
			ssize_t res_tight = rle->decompress(te->input, te->len, tmp_buf, len_check);
			if (res_tight != len_check) {
				TEST_ERRMSG("decompressed output length for tight buffer differs from determined value %zd, got %zd.", len_check, res_tight);
				retval = 1;
			}

			uint32_t res_tight_hash = crc32c((uint32_t)~0, tmp_buf, res_tight) ^ (uint32_t)~0;
			if (res_tight_hash != te->expected_hash) {
				TEST_ERRMSG("expected decompressed hash 0x%08x, got 0x%08x.", te->expected_hash, res_hash);
				retval = 1;
			}

			// Verify there's no content diff between the oversized output buffer and the tight one.
			if (res_tight_hash != res_hash) {
				TEST_ERRMSG("decompressed hash mismatch; 0x%08x vs 0x%08x.", res_tight_hash, res_hash);
				retval = 1;
			}

			if (flag_roundtrip && !no_roundtrip && roundtrip(rle, te, tmp_buf, te->expected_size, 1) != 0) {
				TEST_ERRMSG("re-compressed data does not match original input!");
				retval = 1;
			}

			if ((debug && retval != 0) || hex_always) {
				if (res < 0) {
					fprintf(stdout, "error: %zd -- hexdump unavailable, buffer length unknown!", res);
				} else {
					fprint_hex(stdout, tmp_buf, res, 32, "\n", hex_show_offset);
				}
				printf("\n");
				fflush(stdout);
			}
		} else {
			// end length check/input validation
		}
	} else {
		TEST_ERRMSG("Invalid action");
		retval = 1;
	}

	free(tmp_buf);

	return retval;
}

static int map_file(const char *filename, size_t ofs, ssize_t len, void **data, size_t *size) {
	FILE *f = fopen(filename, "rb");
	if (!f) {
		return 1;
	}

	int res = fseek(f, 0, SEEK_END);
	if (res != 0) {
		fprintf(stderr, "System has broken fseek() -- guess we should have used fstat instead, huh.");
		exit(1);
	}
	size_t flen = ftell(f);

	if (len == 0) {
		len = flen - ofs;
	} else if (len < 0) {
		len = -len;
		ofs = flen - len;
	}
	// printf("map: seek ofs:%zu, len:%zd, flen:%zu\n", ofs, len, flen);
	fseek(f, ofs, SEEK_SET);

	void *base = NULL;
	if (ofs + len <= flen) {
		base = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fileno(f), 0);
		if (base == MAP_FAILED) {
			fclose(f);
			return 2;
		}
		*data = base;
		*size = len;
		fclose(f);
	} else {
		return 3;
	}

	return 0;
}

static int process_file(const char *filename, int depth) {

	if (depth > 3) {
		fprintf(stderr, "Maximum include depth reached -- loop or just silly?\n");
		return -1;
	}

	FILE *f = fopen(filename, "rb");
	if (!f) {
		fprintf(stderr, "ERROR: Could not open input file '%s': %m\n", filename);
		return -1;
	}

	printf("<< Processing '%s':\n", filename);

	char *line = NULL;
	size_t failed_tests = 0;
	size_t line_len = 0;
	size_t line_no = 0;
	ssize_t nread;
	while ((nread = getline(&line, &line_len, f)) != -1) {
		if (nread)
			line[nread - 1] = 0; // Overwrite \n
		++line_no;

		// Skip comments
		if (nread < 3 || line[0] == '#' || line[0] == ';') {
			continue;
		}

		if (strncmp(line, "---", 3) == 0) {
			TEST_WARNMSG("end-marker hit");
			break;
		}

		if (strncmp(line, "include", 7) == 0) {
			int res = process_file(line + 8, depth + 1);
			if (res < 0) {
				failed_tests = -1;
				break;
			}
			failed_tests += res;
			continue;
		}

		// Parse input line
		// goldbox c "AAAAAAAAAAAAAAAA" 2 0xhash
		char *method = NULL;
		char *input = NULL;
		struct test te = {};
		int exsize = 0;
		unsigned int exhash = 0;

		// TODO: Parsing the hex this way is bad, e.g adding a hex digit up front still pass.
		int parsed = sscanf(line, "%ms %ms %ms %i %x", &method, &te.actions, &input, &exsize, &exhash);
		if (parsed >= 3) {
			printf("<< %s\n", line);
			struct rle_t * rle = get_rle_by_name(method);
			if (rle) {
				te.expected_size = exsize;
				te.expected_hash = exhash;

				if (input[0] == '@') {
					// Read input from file.
					void *raw = NULL;
					size_t raw_len = 0;
					ssize_t at_ofs = 0;
					ssize_t at_len = 0;

					int fn_ofs = 1;
					if (input[fn_ofs] == '[') {
						// parse offset + len
						int advance = parse_ofs_len(input + fn_ofs, &at_ofs, &at_len);
						if (advance < 0) {
							TEST_WARNMSG("parse error in range: %d", advance);
							goto nexttest;
						}
						fn_ofs += advance;
					}

					if (map_file(input + fn_ofs, at_ofs, at_len, &raw, &raw_len) != 0) {
						TEST_WARNMSG("file error reading '%s': %m", input+fn_ofs);
						goto nexttest;
					} else {
						te.len = raw_len;
						te.input = malloc(te.len);
						memcpy(te.input, raw, te.len);
						munmap(raw, raw_len);
					}
				} else if (input[0] == '"') {
					int err;
					te.len = expand_escapes(input + 1, strlen(input + 1) - 1, NULL, 0, &err);
					if (err == 0) {
						// NOTE: I intentionally malloc the data, to give valgrind the best chance to detect OOB reads.
						te.input = malloc(te.len);
						te.len = expand_escapes(input + 1, strlen(input + 1) - 1, (char*)te.input, te.len, &err);
						assert(err == 0);
					} else {
						TEST_WARNMSG("invalid escape sequence at position %zu, err %d\n", te.len, err);
						goto nexttest;
					}

				} else {
					TEST_WARNMSG("invalid input format");
					goto nexttest;
				}
				if (run_rle_test(rle, &te, filename, line_no) != 0) {
					++failed_tests;
				}
			} else {
				TEST_WARNMSG("unknown method '%s'", method);
			}

nexttest:
			free(te.input);
			free(te.actions);
		}
		free(input);
		free(method);
	}
	free(line);
	fclose(f);

	return failed_tests;
}

int main(int argc, char *argv[]) {
	const char *filename = argc > 1 ? argv[1] : "all-tests.suite";

	int res = process_file(filename, 1);
	if (res < 0) {
		fprintf(stderr, RED "Test error." NC "\n");
		exit(1);
	}

	if (flag_roundtrip == 0) {
		printf(YELLOW "Warning: Roundtripping disabled -- test coverage decreased!" NC "\n");
	}

	if (res == 0) {
		printf(GREEN "All tests of '%s' passed. (incl. %d roundtrip checks)" NC "\n", filename, num_roundtrip);
	} else {
		fprintf(stderr, RED "%d test failures in suite '%s'." NC "\n", res, filename);
		exit(1);
	}

	return 0;
}
