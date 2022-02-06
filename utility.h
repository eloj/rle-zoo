/*
	Utility Functions
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	See https://github.com/eloj/xxx

	TODO: Move into a separate repo, copy tests from test_utility.c
*/
#define _GNU_SOURCE
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

void dprint_hex(int fd, const uint8_t *data, size_t len, int width, const char *indent, int show_offset);
void fprint_hex(FILE *stream, const uint8_t *data, size_t len, int width, const char *indent, int show_offset);

size_t expand_escapes(const char *input, size_t slen, char *dest, size_t dlen, int *err);

#ifdef UTILITY_IMPLEMENTATION
#include <assert.h>
#include <ctype.h> // for isxdigit()
void dprint_hex(int fd, const uint8_t *data, size_t len, int width, const char *indent, int show_offset) {
	for (size_t i = 0 ; i < len ; ++i) {
		if (show_offset && (i % width == 0)) dprintf(fd, "%08zx: ", i);
		dprintf(fd, "%02x", data[i]);
		if (i < len -1) {
			if (indent && *indent && ((i+1) % width == 0)) {
				dprintf(fd, "%s", indent);
			} else {
				dprintf(fd, " ");
			}
		}
	}
}

void fprint_hex(FILE *stream, const uint8_t *data, size_t len, int width, const char *indent, int show_offset) {
	dprint_hex(fileno(stream), data, len, width, indent, show_offset);
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

#endif

#ifdef __cplusplus
}
#endif