/*
	Run-Length Encoding & Decoding Utility Function Tests
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	See https://github.com/eloj/rle-zoo
*/
#define UTILITY_IMPLEMENTATION
#include "utility.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#define RED "\e[1;31m"
#define GREEN "\e[0;32m"
#define YELLOW "\e[1;33m"
#define NC "\e[0m"

static int debug = 0;
static int debug_hex = 1;

#define TEST_ERRMSG(fmt, ...) \
	fprintf(stderr,"%s:%zu:" RED " error: " NC fmt "\n", testname, i __VA_OPT__(,) __VA_ARGS__)

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
		int err;

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
				fprint_hex(stdout, (const unsigned char*)test->expected_output, strlen(test->expected_output), 32, "\n", 1);
				printf("\ngot:\n");
				fprint_hex(stdout, (const unsigned char*)buf, res, 32, "\n", 1);
				printf("\n");
			}
		}
	}

	if (fails == 0) {
		printf("Suite '%s' passed " GREEN "OK" NC "\n", testname);
	}

	return fails;
}

static int test_parse_ofs_len(void) {
	const char *testname = "parse_ofs_len";
	size_t fails = 0;

	struct parse_test {
		const char *input;
		ssize_t expected_ofs;
		ssize_t expected_len;
		int expected_ret;
	} tests[] = {
		// VALID (ret 0 == input length):
		{ "[]", 0, 0, 0 },
		{ "[:]", 0, 0, 0 },
		{ "[12345678]", 12345678, 0, 0 },
		{ "[12345678:]", 12345678, 0, 0 },
		{ "[12345678:9876543]", 12345678, 9876543, 0 },
		{ "[0x10:0x20]", 16, 32, 0 },
		{ "[12345678:-9876543]", 12345678, -9876543, 0 },
		{ "[:9876543]", 0, 9876543, 0 },
		{ "[:-9876543]", 0, -9876543, 0 },
		// INVALID:
		{ "", 0, 0, -1 },
		{ "[:", 0, 0, -2 },
		{ "[0abba]", 0, 0, -2 },
		{ "[:1abba]", 0, 1, -2 },
		{ "[9999999999999999999]", 0, 0, -3 },
		{ "[:9999999999999999999]", 0, 0, -4 },
	};

	for (size_t i = 0 ; i < sizeof(tests)/sizeof(tests[0]) ; ++i) {
		struct parse_test *test = &tests[i];
		ssize_t res_ofs = 0, res_len = 0;

		int advance = parse_ofs_len(test->input, &res_ofs, &res_len);

		if (test->expected_ret == 0) {
			test->expected_ret = strlen(test->input);
		}

		if (advance != test->expected_ret) {
			TEST_ERRMSG("unexpected return-value, expected '%d', got '%d'.", test->expected_ret, advance);
			++fails;
			continue;
		}

		if (res_ofs != test->expected_ofs) {
			TEST_ERRMSG("parsed offset mismatch, expected '%zd', got '%zd'.", test->expected_ofs, res_ofs);
			++fails;
			continue;
		}
		if (res_len != test->expected_len) {
			TEST_ERRMSG("parsed length mismatch, expected '%zd', got '%zd'.", test->expected_len, res_len);
			++fails;
			continue;
		}

	}

	if (fails == 0) {
		printf("Suite '%s' passed " GREEN "OK" NC "\n", testname);
	}

	return fails;
}

static int test_buf_printf(void) {
	const char *testname = "buf_printf";
	size_t fails = 0;
	size_t i = 0;

	char buf[128];
	size_t wp = 0;
	size_t res;
	int tr = 0;

	buf[7] = 1;
	res = buf_printf(buf, 8, &wp, &tr, "This buffer is %zd bytes", sizeof(buf));
	if (res != 8) {
		TEST_ERRMSG("Undersized buffer, expected res '%zu', got '%zu'.", (size_t)8, res);
		++fails;
	}
	if (wp != 8) {
		TEST_ERRMSG("Undersized buffer, expected wp '%zu', got '%zu'.", (size_t)8, wp);
		++fails;
	}
	if (!tr) {
		TEST_ERRMSG("Undersized buffer, expected truncation flag set, got '%d'.", tr);
		++fails;
	}
	if (buf[7] != 0) {
		TEST_ERRMSG("Undersized buffer, expected zero termination not detected!");
		++fails;
	}

	wp = 0;
	res = buf_printf(buf, sizeof(buf), &wp, NULL, "This buffer is %zd bytes", sizeof(buf));
	if (res != 24) {
		TEST_ERRMSG("Correctly sized buffer, expected '24', got '%zu'.", res);
		++fails;
	}
	if (wp != 24) {
		TEST_ERRMSG("Correctly sized buffer, expected wp '24', got '%zu'.", wp);
		++fails;
	}

	if (memcmp(buf, "This buffer is 128 bytes", 24) != 0) {
		TEST_ERRMSG("Buffer contents failed, got '%s'.", buf);
		++fails;
	}

	wp = -sizeof(buf); // Test write-point under/overflow
	res = buf_printf(buf, sizeof(buf), &wp, NULL, "This buffer is %zd bytes", sizeof(buf));
	if (res != 0) {
		TEST_ERRMSG("Write offset beyond input buffer NOT detected!");
		++fails;
	}

	if (fails == 0) {
		printf("Suite '%s' passed " GREEN "OK" NC "\n", testname);
	}

	return fails;
}

int main(void) {
	size_t failed = 0;

	failed += test_expand_escapes();
	failed += test_parse_ofs_len();
	failed += test_buf_printf();

	if (failed != 0) {
		printf("Tests " RED "FAILED" NC "\n");
	} else {
		printf("All tests " GREEN "passed OK" NC ".\n");
	}

	return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
