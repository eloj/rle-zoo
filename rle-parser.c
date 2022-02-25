/*
	Run-Length Encoding Parser (WIP)
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	See https://github.com/eloj/rle-zoo

	TODO:
		Just give up and use getopt.h
		Take optional offset, length args
		Take debug flag to output ops.
		Log count + ratios of ops, and CPY->REP and REP->CPY transitions.
			e.g PCX decode on packbits input; very obviously wrong because almost only LITs

*/
#define UTILITY_IMPLEMENTATION
#include "utility.h"
#define RLE_PARSE_IMPLEMENTATION
#include "rle-parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#define BUF_SIZE (1024*1024)

#include "ops-packbits.h"
#include "ops-goldbox.h"
#include "ops-pcx.h"
#include "ops-icns.h"

static struct rle8_tbl* rle8_variants[] = {
	&rle8_table_goldbox,
	&rle8_table_packbits,
	&rle8_table_pcx,
	&rle8_table_icns,
};

static int debug_print = 1;
static int debug_hex = 1;

static int rle_parse_encode(struct rle8_tbl *rle, const uint8_t *src, size_t slen) {
	printf("Encoding %zu byte buffer with '%s'\n", slen, rle->name);
	size_t rp = 0;
	size_t wp = 0;
	size_t bailout = 8192;

	struct rle8_params params = {
		rle->minmax_op[RLE_OP_CPY][0],
		rle->minmax_op[RLE_OP_CPY][1],
		rle->minmax_op[RLE_OP_REP][0],
		rle->minmax_op[RLE_OP_REP][1],
	};

	printf("Encode params = { cpy:{ %d, %d }, rep:{%d, %d} }\n", params.min_cpy, params.max_cpy, params.min_rep, params.max_rep);

	if (rle->op_used & (1UL << RLE_OP_LIT)) {
		printf("Unsupported encode table -- LIT scanning support not yet implemented.\n");
		return 0;
	}

	while (rp < slen) {
		struct rle8 res = parse_rle(src + rp, slen - rp, &params);

		if (--bailout == 0) {
			printf("Encode stalled, bailing.\n");
			return -3;
		}

		if (res.op == RLE_OP_REP) {
			int op = rle->encode_tbl[RLE_OP_REP][res.cnt];
			assert(op > -1);
			printf("<%02X> REP '%c' %d\n", op, src[rp], res.cnt);
			rp += res.cnt;
			wp += 2;
			continue;
		}

		if (res.op == RLE_OP_CPY) {
			int op = rle->encode_tbl[RLE_OP_CPY][res.cnt];
			assert(op > -1);
			printf("<%02X> CPY %d\n", op, res.cnt);
			rp += res.cnt;
			wp += res.cnt + 1;
			continue;
		}

		if (res.op == RLE_OP_LIT) {
			int op = rle->encode_tbl[RLE_OP_LIT][res.cnt];
			assert(op > -1);
			printf("<%02X> LIT %d\n", op, res.cnt);
			rp += res.cnt;
			wp += res.cnt;
			continue;
		}
		assert(0 && "Invalid operation returned from parse_rle.");
	}

	printf("rp=%zu, wp=%zu\n", rp, wp);

	return 0;
}

static int rle_parse_decode(struct rle8_tbl *rle, const uint8_t *data, size_t len) {
	printf("Parsing %zu byte buffer with '%s'\n", len, rle->name);
	size_t rp = 0;
	size_t wp = 0;
	size_t bailout = 8192;

	while (rp < len) {
		uint8_t b = data[rp];
		struct rle8 op = rle->decode_tbl[b];

		if (op.op != RLE_OP_INVALID) {
			if (debug_print)
				printf("%08zx: <%02x> %s", rp, b, rle_op_cstr(op.op));
			if (op.op == RLE_OP_CPY) {
				if (debug_print) {
					printf(" %d", op.cnt);
					if (debug_hex) {
						printf(" ; ");
						fflush(stdout);
						fprint_hex(stdout, data + rp + 1, op.cnt, 0, NULL, 0);
					}
				}
				rp += 1 + op.cnt;
				wp += op.cnt;
			} else if (op.op == RLE_OP_REP) {
				if (debug_print)
					printf(" %02x * %d", data[rp+1], op.cnt);
				rp += 2;
				wp += op.cnt;
			} else if (op.op == RLE_OP_LIT) {
				rp += 1;
				wp += 1;
			} else if (op.op == RLE_OP_NOP) {
				rp += 1;
			}
			if (debug_print)
				printf("\n");
		} else {
			printf("%08zu: <%02x> %s\n", rp, b, rle_op_cstr(op.op));
			return -2;
		}

		if (--bailout == 0) {
			printf("Decode stalled, bailing.\n");
			return -3;
		}
	}

	printf("Parse: rp=%zu, wp=%zu\n", rp, wp);
	if (rp != len) {
		return -1;
	}
	// HACKY: Expect at least half the input as output.
	if (wp < len / 2) {
		return -2;
	}

	return 0;
}

int main(int argc, char *argv []) {
	const char *infile = argc > 1 ? argv[1] : "tests/packbits/R128A_C128_R128A.rle";
	int arg_encode = argc > 2 ? atoi(argv[2]) : 0;

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

	struct rle8_tbl *rle = &rle8_table_icns;
	if (arg_encode == 1) {
		rle_parse_encode(rle, buf, p_len);
	} else if (arg_encode == 2) {
		rle = &rle8_table_pcx;
		rle_parse_decode(rle, buf, p_len);
	} else {
		for (size_t i = 0 ; i < sizeof(rle8_variants)/sizeof(rle8_variants[0]) ; ++i) {
			rle = rle8_variants[i];
			int res = rle_parse_decode(rle, buf, p_len);
			if (res == 0) {
				printf("Parse successful.\n");
			} else {
				printf("Parse error: %d\n", res);
			}
		}
	}

	free(buf);

	return EXIT_SUCCESS;
}
