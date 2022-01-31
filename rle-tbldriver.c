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

struct rle8_tbl {
	const char *name;
	const size_t encode_tbl_len;
	const int16_t *encode_tbl[2];
	const struct rle8 *decode_tbl;
	const size_t minmax_op[2][2];
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
