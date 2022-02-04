/*
	Run-Length Encoder/Decoder (RLE), PCX Variant
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	WORK IN PROGRESS -- NOT FULLY VERIFIED IMPLEMENTATION

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
#include <stdio.h> // TEMP

size_t pcx_compress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen) {
	size_t rp = 0;
	size_t wp = 0;

	while (rp < slen && (wp < dlen || dest == NULL)) {
		size_t cnt = 0;
		// size_t cnt = rle_count_rep(src + rp, slen - rp, 63);
		do { ++cnt; } while ((rp + cnt < slen) && (cnt < 63) && (src[rp + cnt - 1] == src[rp + cnt]));

		// Output REP, also include any bytes that can't be encoded as a LIT.
		if (cnt > 1 || ((src[rp] & 0xC0) == 0xC0)) { // or >= 192
			assert(cnt <= 63);
			if (dest && dlen > 0) {
				dest[wp] = 0xC0 | cnt;
				if (wp + 1 < dlen)
					dest[wp + 1] = src[rp];
			}
			wp += 2;
			rp += cnt;
		} else {
			// Output LIT. PERF: Again, this is probably suboptimal.
			if (dest && dlen > 0) {
				dest[wp] = src[rp];
			}
			++rp;
			++wp;
		}
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
