/*
	Run-Length Encoding Parser
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	See https://github.com/eloj/rle-zoo

	TODO:
		Take optional offset, length args
		Take debug flag to output ops.
		Log count + ratios of ops, and CPY->REP and REP->CPY transitions.

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
#include "ops-goldbox.h"

typedef struct rle8 *rle8_decode_tbl;

static rle8_decode_tbl decode_table[] = {
	rle8_tbl_decode_goldbox,
	rle8_tbl_decode_packbits,
};

static const char * decode_table_name[] = {
	"goldbox",
	"packbits",
};

static int debugprint = 0;

static int rle_parse(const uint8_t *data, size_t len, rle8_decode_tbl tbl, const char *variant) {
	printf("Parsing %zu byte buffer with '%s'\n", len, variant);
	size_t rp = 0;
	size_t wp = 0;
	size_t bailout = 8192;

	while (rp < len) {
		uint8_t b = data[rp];
		struct rle8 op = tbl[b];

		if (op.op != RLE_OP_INVALID) {
			if (debugprint)
				printf("%08zx: <%02x> %s %d\n", rp, b, rle_op_cstr(op.op), op.cnt);
			rp++;
			if (op.op == RLE_OP_CPY) {
				rp += op.cnt;
				wp += op.cnt;
			} else if (op.op == RLE_OP_REP) {
				rp += 1;
				wp += op.cnt;
			} else if (op.op == RLE_OP_NOP)
				rp += 1;
		} else {
			printf("%08zu: <%02x> %s\n", rp, b, rle_op_cstr(op.op));
			return -2;
		}

		if (--bailout == 0) {
			printf("Parse stalled, bailing.\n");
			return -3;
		}
	}


	printf("Parse: rp=%zu, wp=%zu\n", rp, wp);
	if (rp != len) {
		return -1;
	}

	return 0;
}

int main(int argc, char *argv []) {
	const char *infile = argc > 1 ? argv[1] : "tests/packbits/R128A_C128_R128A.rle";

	size_t p_offset = 0;
	size_t p_len = BUF_SIZE;

	FILE *f = fopen(infile, "rb");
	if (!f) {
		fprintf(stderr, "Error opening input '%s'\n", infile);
		return EXIT_FAILURE;
	}
	printf("Reading input from '%s'\n", infile);

	uint8_t *buf = malloc(BUF_SIZE);
	if (p_offset) {
		printf("Seeking to offset %08zx.\n", p_offset);
		fseek(f, p_offset, SEEK_SET);
	}
	p_len = fread(buf, 1, p_len, f);
	fclose(f);

	if (p_len == BUF_SIZE) {
		fprintf(stderr, "Error: truncated read, increase BUF_SIZE.\n");
		exit(1);
	}

	for (size_t i = 0 ; i < sizeof(decode_table)/sizeof(decode_table[0]) ; ++i) {
		int res = rle_parse(buf, p_len, decode_table[i], decode_table_name[i]);
		if (res == 0) {
			printf("Parse successful.\n");
		} else {
			printf("Parse error: %d\n", res);
		}
	}

	free(buf);

	return EXIT_SUCCESS;
}