/*
	Run-Length Encoder, Table Driver
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	See https://github.com/eloj/rle-zoo

	TODO:
		Update rle-genops for this new structure (struct rle8_tbl)
		Move into rle-parser instead.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#define BUF_SIZE (1024*1024)

// COPIED FROM GENOPS FOR NOW.
enum RLE_OP {
	RLE_OP_CPY,
	RLE_OP_REP,
	RLE_OP_NOP,
	RLE_OP_INVALID,
};

struct rle8 {
	enum RLE_OP op;
	uint8_t cnt;
};

static const char *rle_op_cstr(enum RLE_OP op) {
	const char *res = "UNKNOWN";
	if (op == RLE_OP_CPY)
		res = "CPY";
	else if (op == RLE_OP_REP)
		res = "REP";
	else if (op == RLE_OP_NOP)
		res = "NOP";
	else if (op == RLE_OP_INVALID)
		res = "INVALID";
	return res;
}

#include "ops-packbits.h"
// #include "ops-goldbox.h"

#ifndef rle8_tbl
struct rle8_tbl {
	const char *name;
	const size_t encode_tbl_len;
	const int16_t *encode_tbl[2];
	const struct rle8 *decode_tbl;
	const size_t minmax_op[2][2];
};
#endif

static struct rle8_tbl rle8_table_packbits = {
	"packbits",
	129,
	{
	 	(int16_t[]){ -1, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f },
		(int16_t[]){ -1, -1, 0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8, 0xf7, 0xf6, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0, 0xef, 0xee, 0xed, 0xec, 0xeb, 0xea, 0xe9, 0xe8, 0xe7, 0xe6, 0xe5, 0xe4, 0xe3, 0xe2, 0xe1, 0xe0, 0xdf, 0xde, 0xdd, 0xdc, 0xdb, 0xda, 0xd9, 0xd8, 0xd7, 0xd6, 0xd5, 0xd4, 0xd3, 0xd2, 0xd1, 0xd0, 0xcf, 0xce, 0xcd, 0xcc, 0xcb, 0xca, 0xc9, 0xc8, 0xc7, 0xc6, 0xc5, 0xc4, 0xc3, 0xc2, 0xc1, 0xc0, 0xbf, 0xbe, 0xbd, 0xbc, 0xbb, 0xba, 0xb9, 0xb8, 0xb7, 0xb6, 0xb5, 0xb4, 0xb3, 0xb2, 0xb1, 0xb0, 0xaf, 0xae, 0xad, 0xac, 0xab, 0xaa, 0xa9, 0xa8, 0xa7, 0xa6, 0xa5, 0xa4, 0xa3, 0xa2, 0xa1, 0xa0, 0x9f, 0x9e, 0x9d, 0x9c, 0x9b, 0x9a, 0x99, 0x98, 0x97, 0x96, 0x95, 0x94, 0x93, 0x92, 0x91, 0x90, 0x8f, 0x8e, 0x8d, 0x8c, 0x8b, 0x8a, 0x89, 0x88, 0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81 },
	},
	rle8_tbl_decode_packbits,
	{
		{ 1, 128 },
		{ 2, 128 },
	}
};

static int debugprint = 0;

// Count the number of repeated characters in the buffer `src` of length `len`, up to the maximum `max`.
// The count is inclusive; for any non-zero length input there's at least one repeated character.
static size_t rle_count_rep(const uint8_t* src, size_t len, size_t max) {
	size_t cnt = 0;
	if (len && max) {
		do { ++cnt; } while ((cnt + 1 <= len) && (cnt < max) && (src[cnt-1] == src[cnt]));
	}
	return cnt;
}

// Count the number of non-repeated characters in the buffer `src` of length `len`, up to the maximum `max`.
static size_t rle_count_cpy(const uint8_t* src, size_t len, size_t max) {
	size_t cnt = 0;
	while ((cnt + 1 <= len) && (cnt < max) && ((cnt + 1 == len) || (src[cnt] != src[cnt+1]))) { ++cnt; };
	return cnt;
}

static int rle_table_encode(struct rle8_tbl *rle, const uint8_t *src, size_t slen) {
	printf("Encoding %zu byte buffer with '%s'\n", slen, rle->name);
	size_t rp = 0;
	size_t wp = 0;
	size_t bailout = 8192;

	size_t min_rep = rle->minmax_op[RLE_OP_REP][0];
	size_t max_rep = rle->minmax_op[RLE_OP_REP][1];
	size_t min_cpy = rle->minmax_op[RLE_OP_CPY][0];
	size_t max_cpy = rle->minmax_op[RLE_OP_CPY][1];

	while (rp < slen) {
		uint8_t cnt = rle_count_rep(src + rp, slen - rp, max_rep);
		if (cnt >= min_rep) {
			assert(cnt < rle->encode_tbl_len);
			// Output RLE_OP_REP <cnt>
			int op = rle->encode_tbl[RLE_OP_REP][cnt];
			assert(op > -1);
			printf("<%02X> REP '%c' %d\n", op, src[rp], cnt);
			rp += cnt;
			wp += 2;
			continue;
		}

		cnt = rle_count_cpy(src + rp, slen - rp, max_cpy);
		if (cnt >= min_cpy) {
			assert(cnt < rle->encode_tbl_len);
			// Output RLE_OP_CNT <cnt>
			int op = rle->encode_tbl[RLE_OP_CPY][cnt];
			assert(op > -1);
			printf("<%02X> CPY %d\n", op, cnt);
			rp += cnt;
			wp += cnt + 1;
		} else {
			printf("MIN CPY FAIL -- Don't know how to make progress.\n");
			return -2;
		}

		if (--bailout == 0) {
			printf("Encode stalled, bailing.\n");
			return -3;
		}
	}

	printf("rp=%zu, wp=%zu\n", rp, wp);

	return 0;
}

int main(int argc, char *argv []) {
	const char *infile = argc > 1 ? argv[1] : "tests/R128A_C128_R128A";

	struct rle8_tbl *rle = &rle8_table_packbits;

	size_t len = BUF_SIZE;

	FILE *f = fopen(infile, "rb");
	if (!f) {
		fprintf(stderr, "Error opening input '%s'\n", infile);
		return EXIT_FAILURE;
	}
	printf("Reading input from '%s'\n", infile);

	uint8_t *buf = malloc(BUF_SIZE);
	len = fread(buf, 1, len, f);
	fclose(f);

	if (len == BUF_SIZE) {
		fprintf(stderr, "Error: truncated read, increase BUF_SIZE.\n");
		exit(1);
	}
	// TODO: Calculate size

	// Compress buf
	rle_table_encode(rle, buf, len);

	free(buf);

	return EXIT_SUCCESS;
}
