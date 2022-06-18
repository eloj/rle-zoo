/*
	Utility Functions
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	See https://github.com/eloj/xxx

	TODO: Move into a separate repo, copy tests from test_utility.c
*/
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>

enum escape_err {
	NO_ERROR,
	ESC_ERROR,
	ESC_ERROR_CHAR,
	ESC_ERROR_HEX,
	ESC_ERROR_DEC,
};

void fprint_hex(FILE *stream, const uint8_t *data, size_t len, int width, const char *indent, int show_offset);

int parse_ofs_len(const char *in, ssize_t *at_ofs, ssize_t *at_len);

size_t expand_escapes(const char *input, size_t slen, char *dest, size_t dlen, int *err);

size_t buf_printf(char *buf, size_t bufsize, size_t *wp, int *truncated, const char *format, ...);

#ifdef UTILITY_IMPLEMENTATION
#include <assert.h>
#include <ctype.h> // for isxdigit()

void fprint_hex(FILE *f, const uint8_t *data, size_t len, int width, const char *indent, int show_offset) {
	for (size_t i = 0 ; i < len ; ++i) {
		if (show_offset && (i % width == 0)) fprintf(f, "%08zx: ", i);
		fprintf(f, "%02x", data[i]);
		if (i < len -1) {
			if (indent && *indent && ((i+1) % width == 0)) {
				fprintf(f, "%s", indent);
			} else {
				fprintf(f, " ");
			}
		}
	}
}

// Parse '[ofs:len]', where ofs is optional and len can be negative.
int parse_ofs_len(const char *in, ssize_t *at_ofs, ssize_t *at_len) {
	int rp = 0;
	char *endptr = NULL;

	if (in[rp++] != '[')
		return -1;

	// Parse offset
	if (isdigit(in[rp])) {
		errno = 0;
		long ofs = strtol(in + rp, &endptr, 0);
		if (errno != 0) {
			return -3;
		}
		rp = endptr - in;
		*at_ofs = ofs;
	}
	if (in[rp] == ':') {
		++rp;
		// Parse length
		if (isdigit(in[rp]) || (in[rp] == '-')) {
			errno = 0;
			long len = strtol(in + rp, &endptr, 0);
			if (errno != 0) {
				return -4;
			}
			rp = endptr - in;
			*at_len = len;
		}
	}

	if (in[rp++] != ']')
		return -2;

	return rp;
}

static uint8_t nibble(const char c) {
	// return c + '0' + (((unsigned)(9 - c) >> 4) & 0x27);
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'a' && c <= 'f') {
		return 10 + c - 'a';
	} else if (c >= 'A' && c <= 'F') {
		return 10 + c - 'A';
	}
	return 0;
}

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
size_t expand_escapes(const char *input, size_t slen, char *dest, size_t dlen, int *err) {
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

// Helper for safe but slow string concatenation. Result is always zero-terminated.
// Returns bytes actually written, updates wp to next write position.
size_t buf_printf(char *buf, size_t bufsize, size_t *wp, int *truncated, const char *format, ...) {
	size_t written = 0;
	int left = 0;

	if (!__builtin_sub_overflow(bufsize, *wp, &left) && left > 0) {
		va_list args;
		va_start(args, format);
		int res = vsnprintf(buf + *wp, left, format, args);
		va_end(args);
		if (res >= 0) {
			if (res >= left) {
				// Output was truncated, return actual number of bytes written.
				res = left;
				if (truncated)
					*truncated = 1;
			}
			written = res;
		}
		// No overflow check here.
		*wp += written;
	}

	return written;
}

#endif

#ifdef __cplusplus
}
#endif
