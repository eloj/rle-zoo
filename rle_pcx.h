/*
	Run-Length Encoder/Decoder (RLE), PCX Variant
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	See https://github.com/eloj/rle-zoo
*/
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

size_t pcx_compress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);
size_t pcx_decompress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);

#ifdef RLE_ZOO_PCX_IMPLEMENTATION
#include <assert.h>

size_t pcx_compress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen) {
	size_t rp = 0;
	size_t wp = 0;

	while (rp < slen && (wp < dlen || dest == NULL)) {
		uint8_t cnt = 0;

		// Count number of same bytes, up to 63

		// Output REP.
		if (cnt > 1) {
			if (dest && dlen > 0) {
			}
			wp += 2;
			rp += cnt;
			continue;
		}

		cnt = 0;
		// Count number of literal bytes, up to 191.

		assert(cnt > 0);
		assert(cnt <= 191);

		// Output LITs
		if (dest && dlen > 0) {
		}
		rp += cnt;
		wp += cnt + 1;
	}
	assert(rp == slen);
	assert((dest == NULL) || (wp <= dlen));
	return wp;
}

size_t pcx_decompress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen) {
	const uint8_t *send = src + slen;
	size_t wp = 0;
	while (src < send && (wp < dlen || dest == NULL)) {
		uint8_t cnt;
		uint8_t b = *src++;

		// REP
		if ((b & 0xC0) == 0xC0) {
			cnt = b & 0x3F;
			if (dest && dlen > 0 && src < send) {
				b = *src++;
				for (int i = 0 ; i < cnt && wp + i < dlen ; ++i)
					dest[wp + i] = b;
			}
			wp += cnt;
		} else {
			// PERF: This is very inefficient. Scan input for repeated literals instead.
			if (dest && dlen > 0)
				dest[wp++] = b;
		}
	}
	assert(src == send);
	assert((dest == NULL) || (wp <= dlen));
	return wp;
}
#endif

#ifdef __cplusplus
}
#endif
