/*
	RLE ZOO single-header library build test harness.
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	This progam exists to verify that all single-header codec libraries
	build with no warnings, under some set of (strict) compiler warning flags.

	See https://github.com/eloj/rle-zoo
*/
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

#define RLE_ZOO_GOLDBOX_IMPLEMENTATION
#include "rle_goldbox.h"
#define RLE_ZOO_PACKBITS_IMPLEMENTATION
#include "rle_packbits.h"
#define RLE_ZOO_PCX_IMPLEMENTATION
#include "rle_pcx.h"

int main(void) {
	const uint8_t input[] = "ABBCCCDDDDEEEEE";
	size_t len = sizeof(input) - 1;

	ssize_t res = 0;

	res += goldbox_compress(input, len, NULL, 0);
	res += packbits_compress(input, len, NULL, 0);
	res += pcx_compress(input, len, NULL, 0);

	printf("%zd bytes required.\n", res);

	return 0;
}
