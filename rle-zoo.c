#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <err.h>

#define RLE_GOLDBOX_IMPLEMENTATION
#include "rle_goldbox.h"

typedef size_t (*rle_fp)(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);

static const char *infile;
static const char *outfile;
static int compress = 0;

static int parse_args(int argc, char **argv) {
	// TODO: just use getopt.h?
	for (int i=1 ; i < argc ; ++i) {
		const char *arg = argv[i];
		// "argv[argc] shall be a null pointer", section 5.1.2.2.1
		const char *value = argv[i+1];

		if (*arg == '-') {
			++arg;
			if (value) {
				switch (*arg) {
					case 'c':
						compress = 1;
						infile = value;
						break;
					case 'd':
						/* fallthrough */
					case 'x':
						compress = 0;
						infile = value;
						break;
					case 'o':
						outfile = value;
						break;
				}
			}
		}
	}

	return 0;
}

static void rle_compress_file(const char *srcfile, const char *destfile, rle_fp compress_func) {
	FILE *ifile = fopen(srcfile, "rb");

	if (!ifile) {
		fprintf(stderr, "Error: %m\n");
		return;
	}
	fseek(ifile, 0, SEEK_END);
	long slen = ftell(ifile);
	fseek(ifile, 0, SEEK_SET);

	printf("Compressing %zu bytes.\n", slen);
	FILE *ofile = fopen(destfile, "wb");
	if (ofile) {
		uint8_t *src = malloc(slen);
		if ((fread(src, slen, 1, ifile) != 1) && (ferror(ifile) != 0)) {
			err(1, "fread: %s", srcfile);
		}

		size_t clen = compress_func(src, slen, NULL, 0);
		if (clen > 0) {
			uint8_t *dest = malloc(clen);
			clen = compress_func(src, slen, dest, clen);

			fwrite(dest, clen, 1, ofile);

			printf("%zd bytes written to output.\n", clen);
		}

		fclose(ofile);
		free(src);
	} else {
		fprintf(stderr, "Error: %m\n");
	}
	fclose(ifile);
}

static void rle_decompress_file(const char *srcfile, const char *destfile, rle_fp decompress_func) {
	FILE *ifile = fopen(srcfile, "rb");

	if (!ifile) {
		fprintf(stderr, "Error: %m\n");
		return;
	}
	fseek(ifile, 0, SEEK_END);
	long slen = ftell(ifile);
	fseek(ifile, 0, SEEK_SET);

	if (slen > 0) {
		printf("Decompressing %zu bytes.\n", slen);
		FILE *ofile = stdout;
		if (strcmp(destfile, "-") != 0) {
			ofile = fopen(destfile, "wb");
		}
		if (ofile) {
			uint8_t *src = malloc(slen);
			if ((fread(src, slen, 1, ifile) != 1) && (ferror(ifile) != 0)) {
				err(1, "fread: %s", srcfile);
			}

			size_t dlen = decompress_func(src, slen, NULL, 0);
			if (dlen > 0) {
				uint8_t *dest = malloc(dlen);
				dlen = decompress_func(src, slen, dest, dlen);

				fwrite(dest, dlen, 1, ofile);

				printf("%zd bytes written to output.\n", dlen);
			}

			fclose(ofile);
			free(src);
		} else {
			fprintf(stderr, "Error: %m\n");
		}
	}
	fclose(ifile);
}

int main(int argc, char *argv []) {

	parse_args(argc, argv);

	if (!infile || !outfile) {
		printf("Usage: %s -c file|-d file -o outfile\n", argv[0]);
		return EXIT_SUCCESS;
	}

	// No options yet, so just pick the one we have.
	rle_fp compress_func = goldbox_compress;
	rle_fp decompress_func = goldbox_decompress;

	printf("rle-zoo %s file %s\n", compress ? "compressing" : "decompressing", infile);
	if (compress) {
		rle_compress_file(infile, outfile, compress_func);
	} else {
		rle_decompress_file(infile, outfile, decompress_func);
	}

	return EXIT_SUCCESS;
}
