/*
	RLE ZOO single-header library example.
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	See https://github.com/eloj/rle-zoo
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RLE_ZOO_PACKBITS_IMPLEMENTATION
#include "rle_packbits.h"

int main(void) {
	const uint8_t input[] = "ABBBBA";
	size_t len = sizeof(input) - 1;

	// Call with NULL for dest buffer and size to calculate output size.
	size_t compressed_size = packbits_compress(input, len, NULL, 0);

	if (compressed_size > 0) {
		uint8_t *output = malloc(compressed_size);
		if (output) {
			packbits_compress(input, len, output, compressed_size);
			printf("Compressed '%s' into %zu bytes: ", input, compressed_size);

			// Do something with the output.
			for (size_t i = 0 ; i < compressed_size ; ++i) {
				printf("%02X ", output[i]);
			}
			printf("\n");

			free(output);
		}
	}

	return 0;
}
