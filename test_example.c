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
	const uint8_t input[] = "ABBCCCDDDDEEEEE";
	size_t len = sizeof(input) - 1;

	// Call with NULL for dest buffer to calculate output size.
	ssize_t compressed_size = packbits_compress(input, len, NULL, 0);

	if (compressed_size > 0) {
		uint8_t *output = malloc(compressed_size);
		if (output) {
			ssize_t res = packbits_compress(input, len, output, compressed_size);
			if (res > 0) {
				printf("Compressed '%s' (%zu bytes) into %zd bytes: ", input, len, compressed_size);

				// Do something with the output.
				for (int i = 0 ; i < res ; ++i) {
					printf("%02X ", output[i]);
				}
				printf("\n");
			} else {
				printf("Compression error: %zd\n", res);
			}
			free(output);
		}
	}
	return 0;
}
