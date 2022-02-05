/*
	Run-Length Encoding & Decoding Utility Function Tests
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	See https://github.com/eloj/rle-zoo
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define RED "\e[1;31m"
#define GREEN "\e[0;32m"
#define YELLOW "\e[1;33m"
#define NC "\e[0m"

static int debug = 0;
static int debug_hex = 1;

// Copied from test_rle.c:
static void print_hex(const uint8_t *data, size_t len, int width, const char *indent, int show_offset) {
	for (size_t i = 0 ; i < len ; ++i) {
		if (show_offset && (i % width == 0)) printf("%08zx: ", i);
		printf("%02x", data[i]);
		if (i < len -1) {
			if (indent && *indent && ((i+1) % width == 0)) {
				printf("%s", indent);
			} else {
				printf(" ");
			}
		}
	}
}

/*
	A -> 1
	AA -> 2
	AB -> 1
*/
// Count the number of repeated characters in the buffer `src` of length `len`, up to the maximum `max`.
// The count is inclusive; for any non-zero length input there's at least one repeated character.
static size_t rle_count_rep(const uint8_t* src, size_t len, size_t max) {
	size_t cnt = 0;
	if (len && max) {
		do { ++cnt; } while ((cnt + 1 <= len) && (cnt < max) && (src[cnt-1] == src[cnt]));
	}
	return cnt;
}

/*
	A -> 1
	AA -> 0 (<--!)
	AB -> 2
	ABB -> 1
*/
// Count the number of non-repeated characters in the buffer `src` of length `len`, up to the maximum `max`.
static size_t rle_count_cpy(const uint8_t* src, size_t len, size_t max) {
	size_t cnt = 0;
	while ((cnt + 1 <= len) && (cnt < max) && ((cnt + 1 == len) || (src[cnt] != src[cnt+1]))) { ++cnt; };
	return cnt;
}

static uint8_t* make_rep(int ch, size_t n) {

	uint8_t *res = malloc(n);
	if (n) {
		memset(res, ch, n);
	}
	return res;
}

static uint8_t* make_cpy(int ch, size_t n) {

	uint8_t *res = malloc(n);
	if (n) {
		for (size_t i = 0 ; i < n ; ++i) {
			res[i] = ((i & 1) == 0) ? ch : ch+1;
		}
	}
	return res;
}

#define TEST_ERRMSG(fmt, ...) \
	fprintf(stderr,"%s:%zu:" RED " error: " NC fmt "\n", testname, i __VA_OPT__(,) __VA_ARGS__)

static int test_rep(void) {
	const char *testname = "REP";
	size_t fails = 0;
	for (size_t i=0 ; i <= 16 ; ++i) {
		uint8_t *arr = make_rep('A', i);

		// Test limiter
		size_t rep0 = rle_count_rep(arr, i, i / 2);
		// Test scanning up to the end
		size_t rep1 = rle_count_rep(arr, i, i);
		// Test not overrunning the end
		size_t rep2 = rle_count_rep(arr, i, i * 2);

		size_t cpy0 = rle_count_cpy(arr, i, i);

		if (debug) {
			printf("REP: %zu: rep0=%zu (max=%zu), rep1=%zu (max=%zu), rep2=%zu (max=%zu)   --- ", i, rep0, i / 2, rep1, i, rep2, i * 2);
			printf("CPY: %zu: cpy0=%zu (max=%zu)\n", i, cpy0, i);
		}

		if (!(rep0 == i / 2)) {
			TEST_ERRMSG("REP %zu -- Count does not match max limit.", i);
			++fails;
		}
		if (!(rep1 == rep2)) {
			TEST_ERRMSG("REP %zu -- Count does not match repeated input length.", i);
			++fails;
		}
		if (!(rep2 == i)) {
			TEST_ERRMSG("REP %zu -- Count does not match repeated input length (high limit).", i);
			++fails;
		}
		if (!((i==1 && cpy0 == 1) || cpy0 == 0)) {
			if (i==1 && cpy0 != 1)
				TEST_ERRMSG("CPY %zu -- Should be 1 for one-length input, got %zu", i, cpy0);
			else
				TEST_ERRMSG("CPY %zu -- Should be zero for repeating inputs, got %zu.", i, cpy0);
			++fails;
		}

		if (fails) {
			printf("INPUT: '%.*s'\n", (int)i, arr);
			free(arr);
			break;
		}

		free(arr);
	}

	{
		size_t i = 0;
		size_t rep0 = rle_count_rep((const uint8_t*)"BBBBA", 5, 128);
		if (rep0 != 4) {
			TEST_ERRMSG("REP should be 4 for BBBBA, got %zu", rep0);
			++fails;
		}
	}

	if (fails == 0) {
		printf("Suite '%s' passed " GREEN "OK" NC "\n", testname);
	}

	return fails;
}

static int test_cpy(void) {
	const char *testname = "CPY";
	size_t fails = 0;
	for (size_t i=0 ; i <= 16 ; ++i) {
		uint8_t *arr = make_cpy('A', i);

		// Test limiter
		size_t cpy0 = rle_count_cpy(arr, i, i / 2);
		// Test scanning up to the end
		size_t cpy1 = rle_count_cpy(arr, i, i);
		// Test not overrunning the end
		size_t cpy2 = rle_count_cpy(arr, i, i * 2);

		size_t rep0 = rle_count_rep(arr, i, i);

		if (debug) {
			printf("CPY: %zu: cpy0=%zu (max=%zu), cpy1=%zu (max=%zu), cpy2=%zu (max=%zu)   --- ", i, cpy0, i / 2, cpy1, i, cpy2, i * 2);
			printf("REP: %zu: rep0=%zu (max=%zu)\n", i, rep0, i);
		}

		if (!(cpy0 == i / 2)) {
			TEST_ERRMSG("CPY %zu -- Count does not match max limit.", i);
			++fails;
		}
		if (!(cpy1 == i)) {
			TEST_ERRMSG("CPY %zu -- Count does not match input length.", i);
			++fails;
		}
		if (!(cpy2 == cpy1 && cpy2 == i)) {
			TEST_ERRMSG("CPY %zu -- Count does not match input length (high limit).", i);
			++fails;
		}
		if (!((i==0 && rep0 == 0) || rep0 == 1)) {
			if (i==0 && rep0 != 0)
				TEST_ERRMSG("REP %zu -- Should be zero for zero-length input, got %zu", i, rep0);
			else
				TEST_ERRMSG("REP %zu -- Should be one for non-repeating inputs, got %zu.", i, rep0);
			++fails;
		}

		if (fails) {
			printf("INPUT: '%.*s'\n", (int)i, arr);
			free(arr);
			break;
		}

		free(arr);
	}

	{
		size_t i = 0;
		size_t cpy0 = rle_count_cpy((const uint8_t*)"AB", 2, 128);
		if (cpy0 != 2) {
			TEST_ERRMSG("CPY should be one for AB, got %zu", cpy0);
			++fails;
		}

		cpy0 = rle_count_cpy((const uint8_t*)"ABB", 3, 128);
		if (cpy0 != 1) {
			TEST_ERRMSG("CPY should be one for ABB, got %zu", cpy0);
			++fails;
		}
	}


	if (fails == 0) {
		printf("Suite '%s' passed " GREEN "OK" NC "\n", testname);
	}

	return fails;
}

static uint8_t nibble(const char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'a' && c <= 'f') {
		return 10 + c - 'a';
	} else if (c >= 'A' && c <= 'F') {
		return 10 + c - 'A';
	}
	return 0;
}

enum escape_err {
	NO_ERROR,
	ESC_ERROR,
	ESC_ERROR_CHAR,
	ESC_ERROR_HEX,
	ESC_ERROR_DEC,
};

// Expand escape codes. Not compatible with stdlib!
//
// Accepts:
//  - Standard escapes: \r\n\t, etc.
//  - Hexadecimal escapes: \x00 - \xFF
//  - Decimal escapes: \0 - \255
//  No support of octal.
//
// Returns
//   If dest is NULL, then returns number of bytes that WOULD be written.
//   On success:
//   	Returns number of bytes written.
//   On error:
//   	Sets err, and returns position of error in input.
static size_t expand_escapes(const char *input, size_t slen, char *dest, size_t dlen, int *err) {
	size_t rp = 0;
	size_t wp = 0;

#define RETURN_ERR(err_enum) { if (err) *err = err_enum; return rp_start; } while (0)
	assert(err != NULL);

	while (rp < slen && (wp < dlen || dest == NULL)) {
		// Use start of scan as return value on error.
		size_t rp_start = rp;
		char c = input[rp++];
		if (c == '\\') {
			// Check if dangling escape
			if (rp >= slen)
				RETURN_ERR(ESC_ERROR);

			if (input[rp] == 'x') {
				// HEX escape
				if (rp + 2 < slen && isxdigit(input[rp+1]) && isxdigit(input[rp+2])) {
					c = (nibble(input[rp+1]) << 4) | nibble(input[rp+2]);
					rp += 3;
				} else {
					RETURN_ERR(ESC_ERROR_HEX);
				}
			} else if (isdigit(input[rp])) {
				// DECimal escape
				int decval = input[rp++] - '0';
				if (rp < slen && isdigit(input[rp])) {
					decval *= 10;
					decval += input[rp++] - '0';
				}
				if (rp < slen && isdigit(input[rp])) {
					decval *= 10;
					decval += input[rp++] - '0';
				}
				if (decval > 255) {
					RETURN_ERR(ESC_ERROR_DEC);
				}
				c = decval;
			} else {
				// Standard escape character
				// We don't support \? because that is a trigraphs legacy.
				switch (input[rp++]) {
					case 'a': c = '\a'; break;
					case 'b': c = '\b'; break;
					case 'f': c = '\f'; break;
					case 'n': c = '\n'; break;
					case 'r': c = '\r'; break;
					case 't': c = '\t'; break;
					case 'v': c = '\v'; break;
					case '"': c = '"';  break;
					case '\\': c = '\\';break;
					default:
						RETURN_ERR(ESC_ERROR_CHAR);
				}
			}
		}

		if (dest && dlen) {
			if (wp < dlen) {
				dest[wp] = c;
			}
		}
		++wp;
	}
	if (err)
		*err = 0;
	return wp;
#undef RETURN_ERR
}

struct escape_test {
	const char *input;
	const char *expected_output;
	size_t expected_len;
	int expected_err;
};

static int test_expand_escapes(void) {
	const char *testname = "expand_escapes";
	size_t fails = 0;
	char buf[1024];

	struct escape_test tests[] = {
		// Expected pass tests:
		{ "", "", 0, 0 },
		{ "A", "A", 1, 0 },
		{ "\\xFF", "\xFF", 1, 0 },
		{ "A\\x40A", "A@A", 3, 0 },
		{ "\\0", "\0", 1, 0 },
		{ "\\1\\32\\128", "\1\40\200", 3, 0 },
		{ "\\\"", "\"", 1, 0 },
		{ "\\a\\b\\f\\n\\r\\t\\v", "\a\b\f\n\r\t\v", 7, 0 },
		// Expected error tests:
		{ "\\", "", 0, ESC_ERROR },
		{ "\\x", "", 0, ESC_ERROR_HEX },
		{ "\\x8", "", 0, ESC_ERROR_HEX }, // NOTE: Should perhaps accept as extension?
		{ "\\xfz", "", 0, ESC_ERROR_HEX },
		{ "\\256", "", 0, ESC_ERROR_DEC },
		{ "\\?", "", 0, ESC_ERROR_CHAR },
	};

	for (size_t i = 0 ; i < sizeof(tests)/sizeof(tests[0]) ; ++i) {
		struct escape_test *test = &tests[i];
		int err, err2;

		size_t res_len = expand_escapes(test->input, strlen(test->input), NULL, 0, &err);
		if (err != test->expected_err) {
			TEST_ERRMSG("unexpected error, expected '%d', got '%d' (position %zu).", test->expected_err, err, res_len);
			++fails;
			continue;
		}
		if (test->expected_err != 0) {
			// Expected error -- we're done here.
			continue;
		}

		if (res_len != test->expected_len) {
			TEST_ERRMSG("length-determination result mismatch, expected '%zu', got '%zu'.", test->expected_len, res_len);
			++fails;
			continue;
		}

		size_t res = expand_escapes(test->input, strlen(test->input), buf, sizeof(buf), &err);
		if (res != res_len) {
			TEST_ERRMSG("output length differs, expected '%zu', got '%zu'.", res_len, res);
			++fails;
			continue;
		}

		if (memcmp(buf, test->expected_output, res) != 0) {
			TEST_ERRMSG("output buffer contents mismatch.");
			if (debug_hex) {
				printf("expected:\n");
				print_hex((const unsigned char*)test->expected_output, strlen(test->expected_output), 32, "\n", 1);
				printf("\ngot:\n");
				print_hex((const unsigned char*)buf, res, 32, "\n", 1);
				printf("\n");
			}
		}
	}

	if (fails == 0) {
		printf("Suite '%s' passed " GREEN "OK" NC "\n", testname);
	}

	return fails;
}

int main(int argc, char *argv[]) {
	size_t failed = 0;

	failed += test_expand_escapes();
	failed += test_rep();
	failed += test_cpy();

	if (failed != 0) {
		printf("Tests " RED "FAILED" NC "\n");
	} else {
		printf("All tests " GREEN "passed OK" NC ".\n");
	}

	return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
