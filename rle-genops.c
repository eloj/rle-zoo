#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

typedef struct rle8 (*rle8_decode_fp)(uint8_t input);
typedef uint8_t (*rle8_encode_fp)(struct rle8 cmd);

struct rle_parser {
	const char *name;
	rle8_encode_fp rle8_encode;
	rle8_decode_fp rle8_decode;
};

enum RLE_OP {
	RLE_OP_CPY,
	RLE_OP_REP,
	RLE_OP_NOP
};

struct rle8 {
	enum RLE_OP op;
	uint8_t cnt;
};

static const char *rle_op_cstr(enum RLE_OP op) {
	const char *res = "UNKNOWN";
	if (op == RLE_OP_CPY)
		res = "CPY";
	if (op == RLE_OP_REP)
		res = "REP";
	if (op == RLE_OP_NOP)
		res = "NOP";
	return res;
}

static struct rle8 rle8_decode_packbits(uint8_t input) {
	struct rle8 cmd;

	if (input > 128) {
		cmd.op = RLE_OP_REP;
		cmd.cnt = 1 - (int8_t)input;
	} else if (input < 128) {
		cmd.op = RLE_OP_CPY;
		cmd.cnt = input + 1;
	} else {
		cmd.op = RLE_OP_NOP;
		cmd.cnt = 1;
	}

	assert(cmd.cnt > 0);
	assert(cmd.cnt <= 128);

	return cmd;
}

static uint8_t rle8_encode_packbits(struct rle8 cmd) {
	uint8_t res;

	assert(cmd.op == RLE_OP_CPY || cmd.op == RLE_OP_REP || cmd.op == RLE_OP_NOP);
	assert(cmd.cnt > 0);
	assert(cmd.cnt <= 128);

	if (cmd.op == RLE_OP_REP) {
		res = 257 - cmd.cnt; // 1 - (int8_t)cmd.cnt;
	} else if (cmd.op == RLE_OP_CPY) {
		res = cmd.cnt - 1;
	} else if (cmd.op == RLE_OP_NOP) {
		res = 0x80;
	}

	return res;
}

static struct rle8 rle8_decode_goldbox(uint8_t input) {
	struct rle8 cmd;

	if (input & 0x80) {
		cmd.op = RLE_OP_REP;
		cmd.cnt = (~input) + 1;
	} else {
		cmd.op = RLE_OP_CPY;
		cmd.cnt = input + 1;
	}

	assert(cmd.cnt > 0);
	assert(cmd.cnt <= 128);

	return cmd;
}

static uint8_t rle8_encode_goldbox(struct rle8 cmd) {
	uint8_t res;

	assert(cmd.op == RLE_OP_CPY || cmd.op == RLE_OP_REP);
	assert(cmd.cnt > 0);
	assert(cmd.cnt <= 128);

	if (cmd.op == RLE_OP_REP) {
		res = 1 + (~cmd.cnt);
	} else if (cmd.op == RLE_OP_CPY) {
		res = cmd.cnt - 1;
	}

	return res;
}

static int rle8_generate_ops(struct rle_parser *p) {
	printf("# Automatically generated code table for RLE8 variant '%s'\n", p->name);
	for (int i=0 ; i < 256 ; ++i) {
		uint8_t b = i;

		struct rle8 cmd = p->rle8_decode(b);
		uint8_t b_recode = p->rle8_encode(cmd);

		printf("0x%02x (%d/%d) => %s %d\n", b, b, (int8_t)b, rle_op_cstr(cmd.op), cmd.cnt);

		if (b != b_recode) {
			printf("ERROR: reencode mismatch: %s %d => 0x%02x\n", rle_op_cstr(cmd.op), cmd.cnt, b_recode);
			return 1;
		}
	}
	return 0;
}

struct rle_parser parsers[] = {
	{
		"goldbox",
		rle8_encode_goldbox,
		rle8_decode_goldbox
	},
	{
		"packbits",
		rle8_encode_packbits,
		rle8_decode_packbits
	}
};
static const size_t NUM_VARIANTS = sizeof(parsers)/sizeof(parsers[0]);

static struct rle_parser* get_parser_by_name(const char *name) {
	for (size_t i = 0 ; i < NUM_VARIANTS ; ++i) {
		if (strcmp(name, parsers[i].name) == 0) {
			return &parsers[i];
		}
	}
	return NULL;
}

static void usage(const char *argv) {
	printf("%s [<variant>]\n\n", argv);

	printf("Available variants:\n");
	struct rle_parser *p = parsers;
	for (size_t i = 0 ; i < NUM_VARIANTS ; ++i) {
		printf("  %s\n", p->name);
		++p;
	}
}

int main(int argc, char *argv[]) {
	struct rle_parser *p = argc > 1 ? get_parser_by_name(argv[1]) : NULL;

	if (!p) {
		usage(argv[0]);
		return 1;
	}

	rle8_generate_ops(p);

	return 0;
}
