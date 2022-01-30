/*
	Run-Length Encoder/Decoder (RLE), Packbits Variant
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	See https://github.com/eloj/rle-zoo
*/
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

size_t packbits_compress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);
size_t packbits_decompress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);

#ifdef RLE_ZOO_PACKBITS_IMPLEMENTATION
#include <assert.h>

// RLE PARAMS: min CPY=1, max CPY=128, min REP=2, max REP=128
size_t packbits_compress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen) {
	size_t rp = 0;
	size_t wp = 0;

	while (rp < slen && (wp < dlen || dest == NULL)) {
		uint8_t cnt = 0;

		// Count number of same bytes, up to 128
		while ((rp+cnt+1 < slen) && (src[rp+cnt] == src[rp+cnt+1]) && (cnt < 127)) {
			++cnt;
		}
		++cnt;

		// Output REP.
		if (cnt > 1) {
			assert(cnt >= 2 && cnt <= 128);
			if (dest && dlen > 0) {
				dest[wp+0] = 257 - cnt;
				if (wp + 1 < dlen)
					dest[wp+1] = src[rp];
			}
			wp += 2;
			rp += cnt;
			continue;
		}

		cnt = 0;
		// Count number of literal bytes, up to 128.
		while ((rp+cnt+1 <= slen) && (cnt < 128) && ((rp+cnt+1 == slen) || (src[rp+cnt] != src[rp+cnt+1])))
			++cnt;

		assert(cnt > 0);
		assert(cnt <= 128);

		// Output CPY
		if (dest && dlen > 0) {
			dest[wp+0] = cnt - 1;
			// memcpy(dest+wp+1, src+rp, cnt);
			for (int i = 0 ; i < cnt && wp + 1 + i < dlen ; ++i)
				dest[wp + 1 + i] = src[rp + i];
		}
		rp += cnt;
		wp += cnt + 1;
	}
	assert(rp == slen);
	assert((dest == NULL) || (wp <= dlen));
	return wp;
}

size_t packbits_decompress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen) {
	const uint8_t *send = src + slen;
	size_t wp = 0;
	while (src < send && (wp < dlen || dest == NULL)) {
		uint8_t cnt;
		uint8_t b = *src++;
		if (b > 0x80) {
			// REP
			cnt = 257 - b;
			if (dest && dlen > 0) {
				// memset(dest + wp, *src, cnt);
				for (int i = 0 ; i < cnt && wp + i < dlen ; ++i)
					dest[wp + i] = *src;
			}
			++src;
		} else if (b < 0x80) {
			// CPY
			cnt = b + 1;
			if (dest && dlen > 0) {
				// memcpy(dest + wp, src, cnt);
				for (int i = 0 ; i < cnt && wp + i < dlen ; ++i)
					dest[wp + i] = src[i];
			}
			src += cnt;
		} else {
			// 0x80 is reserved, skip byte as suggested by TN1023.
			++src;
			cnt = 0;
		}
		wp += cnt;
	}
	assert(src == send);
	assert((dest == NULL) || (wp <= dlen));
	return wp;
}
#endif

#ifdef __cplusplus
}
#endif
