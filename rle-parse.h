/*
	RLE Parsing Utility Functions
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	TODO:
		The parser is incomplete and not fully functional.
		Add support for LIT.

	See https://github.com/eloj/rle-zoo
*/
#ifdef __cplusplus
extern "C" {
#endif

// NOTE: The order and value of these matter.
enum RLE_OP {
	RLE_OP_CPY,
	RLE_OP_REP,
	RLE_OP_LIT,
	RLE_OP_NOP,
	RLE_OP_INVALID,
};

struct rle8 {
	enum RLE_OP op;
	uint8_t cnt; // TODO: rename to 'arg'?
};

struct rle8_tbl {
	const char *name;
	enum RLE_OP op_used;
	const int16_t *encode_tbl[3];
	const struct rle8 *decode_tbl;
	const size_t minmax_op[3][2];
};

struct rle8_params {
	int min_cpy;
	int max_cpy;
	int min_rep;
	int max_rep;
};

const char *rle_op_cstr(enum RLE_OP op);
struct rle8 parse_rle(const uint8_t *in, size_t len, const struct rle8_params *params);

size_t rle_count_rep(const uint8_t* src, size_t len, size_t max);
size_t rle_count_cpy(const uint8_t* src, size_t len, size_t max);

#ifdef RLE_PARSE_IMPLEMENTATION

const char *rle_op_cstr(enum RLE_OP op) {
	const char *res = "UNKNOWN";
	switch (op) {
		case RLE_OP_CPY:
			res = "CPY";
			break;
		case RLE_OP_REP:
			res = "REP";
			break;
		case RLE_OP_LIT:
			res = "LIT";
			break;
		case RLE_OP_NOP:
			res = "NOP";
			break;
		case RLE_OP_INVALID:
			res = "INVALID";
			break;
	}
	return res;
}


struct rle8 parse_rle(const uint8_t *in, size_t len, const struct rle8_params *params) {
	struct rle8 res = { RLE_OP_CPY, 0 };

	const uint8_t *p = in;
	uint8_t prev = *p;
	int num_same = 0;
	int num_scan = 0;

	while (p < in + len) {
		++num_scan;
		if (*p == prev) {
			++num_same;
		} else {
			// Check end of REP.
			if (num_same >= params->min_rep) {
				return (struct rle8){ RLE_OP_REP, num_same };
			}
			num_same = 1;
		}
		// printf("<'%c' scan:%d, same:%d\n", *p, num_scan, num_same);

		// Check for min-rep violation while in CPY scan
		if (num_same == params->min_rep && num_scan != num_same) {
			return (struct rle8){ RLE_OP_CPY, num_scan - num_same };
		}

		// Check for max-cpy limit
		if (num_scan == params->max_cpy && (num_scan != num_same)) {
			return (struct rle8){ RLE_OP_CPY, num_scan };
		}

		// Check for max-rep limit
		if (num_same == params->max_rep && num_scan == num_same) {
			return (struct rle8){ RLE_OP_REP, num_same };
		}

		prev = *p;
		++p;
	}
	if (num_same == num_scan && num_same >= params->min_rep) {
		res.op = RLE_OP_REP;
	}

	res.cnt = num_scan;

	return res;
}

/*
	A -> 1
	AA -> 2
	AB -> 1
*/
// Count the number of repeated characters in the buffer `src` of length `len`, up to the maximum `max`.
// The count is inclusive; for any non-zero length input there's at least one repeated character.
size_t rle_count_rep(const uint8_t* src, size_t len, size_t max) {
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
size_t rle_count_cpy(const uint8_t* src, size_t len, size_t max) {
	size_t cnt = 0;
	while ((cnt + 1 <= len) && (cnt < max) && ((cnt + 1 == len) || (src[cnt] != src[cnt+1]))) { ++cnt; };
	return cnt;
}

#endif

#ifdef __cplusplus
}
#endif

