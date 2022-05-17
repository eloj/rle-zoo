/*
	Shared Run-Length Encoding & Decoding Driver Selection Code
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	See https://github.com/eloj/rle-zoo
*/
#include <string.h>

typedef ssize_t (*rle_fp)(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);

struct rle_t {
	const char *name;
	rle_fp compress;
	rle_fp decompress;
} rle_variants[] = {
	{
		.name = "goldbox",
		.compress = goldbox_compress,
		.decompress = goldbox_decompress
	},
	{
		.name = "packbits",
		.compress = packbits_compress,
		.decompress = packbits_decompress
	},
	{
		.name = "pcx",
		.compress = pcx_compress,
		.decompress = pcx_decompress
	},
	{
		.name = "icns",
		.compress = icns_compress,
		.decompress = icns_decompress
	},
};

static const size_t RLE_ZOO_NUM_VARIANTS = sizeof(rle_variants)/sizeof(rle_variants[0]);

static struct rle_t* get_rle_by_name(const char *name) {
	for (size_t i = 0 ; i < RLE_ZOO_NUM_VARIANTS ; ++i) {
		if (strcmp(name, rle_variants[i].name) == 0) {
			return &rle_variants[i];
		}
	}
	return NULL;
}

static void print_variants(void) {
	printf("\nAvailable variants:\n");
	struct rle_t *rle = rle_variants;
	for (size_t i = 0 ; i < RLE_ZOO_NUM_VARIANTS ; ++i) {
		printf("  %s\n", rle->name);
		++rle;
	}
}
