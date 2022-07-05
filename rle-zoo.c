/*
	Run-Length Encoding & Decoding Driver
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	See https://github.com/eloj/rle-zoo
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#define RLE_ZOO_IMPLEMENTATION
#include "rle_goldbox.h"
#include "rle_packbits.h"
#include "rle_pcx.h"
#include "rle_icns.h"

#include "rle-variant-selection.h"

#include "build_const.h"

static const char *infile;
static const char *outfile;
static const char *variant;
static int compress = 0;

static void print_banner(void) {
	printf("rle-zoo %s <%.*s>\n", build_version, 8, build_hash);
}

static int parse_args(int argc, char **argv) {
	// TODO: just use getopt.h?
	for (int i=1 ; i < argc ; ++i) {
		const char *arg = argv[i];
		// "argv[argc] shall be a null pointer", section 5.1.2.2.1
		const char *value = argv[i+1];

		if (arg && *arg == '-') {
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
					case 't':
						variant = value;
						break;
				}
			} else {
				if (*arg == 'v' || *arg == 'V' || strcmp(arg, "-version") == 0) {
					print_banner();
					exit(0);
				}
			}
		}
	}

	return 0;
}

static void rle_compress_file(const char *srcfile, const char *destfile, rle_fp compress_func) {
	FILE *ifile = fopen(srcfile, "rb");

	if (!ifile) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return;
	}
	fseek(ifile, 0, SEEK_END);
	long slen = ftell(ifile);
	fseek(ifile, 0, SEEK_SET);

	printf("Compressing %lu bytes.\n", slen);
	FILE *ofile = fopen(destfile, "wb");
	if (ofile) {
		uint8_t *src = malloc(slen);
		if ((fread(src, slen, 1, ifile) != 1) && (ferror(ifile) != 0)) {
			fprintf(stderr, "%s: fread: %s: %s", __FILE__, srcfile, strerror(errno));
			exit(EXIT_FAILURE);
		}

		ssize_t clen = compress_func(src, slen, NULL, 0);
		if (clen >= 0) {
			uint8_t *dest = malloc(clen);
			clen = compress_func(src, slen, dest, clen);

			fwrite(dest, clen, 1, ofile);

			printf("%zd bytes written to output.\n", clen);
		} else {
			printf("Compression error: %zd\n", clen);
		}

		fclose(ofile);
		free(src);
	} else {
		fprintf(stderr, "Error: %s\n", strerror(errno));
	}
	fclose(ifile);
}

static void rle_decompress_file(const char *srcfile, const char *destfile, rle_fp decompress_func) {
	FILE *ifile = fopen(srcfile, "rb");

	if (!ifile) {
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return;
	}
	fseek(ifile, 0, SEEK_END);
	long slen = ftell(ifile);
	fseek(ifile, 0, SEEK_SET);

	if (slen > 0) {
		printf("Decompressing %lu bytes.\n", slen);
		FILE *ofile = stdout;
		if (strcmp(destfile, "-") != 0) {
			ofile = fopen(destfile, "wb");
		}
		if (ofile) {
			uint8_t *src = malloc(slen);
			if ((fread(src, slen, 1, ifile) != 1) && (ferror(ifile) != 0)) {
				fprintf(stderr, "%s: fread: %s: %s", __FILE__, srcfile, strerror(errno));
				exit(EXIT_FAILURE);
			}

			ssize_t dlen = decompress_func(src, slen, NULL, 0);
			if (dlen >= 0) {
				uint8_t *dest = malloc(dlen);
				dlen = decompress_func(src, slen, dest, dlen);

				fwrite(dest, dlen, 1, ofile);

				printf("%zd bytes written to output.\n", dlen);
			} else {
				printf("Decompression error: %zd\n", dlen);
			}

			fclose(ofile);
			free(src);
		} else {
			fprintf(stderr, "Error: %s\n", strerror(errno));
		}
	}
	fclose(ifile);
}

int main(int argc, char *argv []) {

	parse_args(argc, argv);

	print_banner();

	if (!infile || !outfile || !variant) {
		printf("Usage: %s -t variant -c file|-d file -o outfile\n", argv[0]);
		print_variants();
		return EXIT_SUCCESS;
	}

	struct rle_t* rle = get_rle_by_name(variant);
	if (!rle) {
		print_variants();
		fprintf(stderr, "ERROR: Unknown variant '%s'.\n", variant);
		return EXIT_FAILURE;
	}

	printf("rle-zoo %s file '%s' with variant '%s'\n", compress ? "compressing" : "decompressing", infile, rle->name);
	if (compress) {
		rle_compress_file(infile, outfile, rle->compress);
	} else {
		rle_decompress_file(infile, outfile, rle->decompress);
	}

	return EXIT_SUCCESS;
}
