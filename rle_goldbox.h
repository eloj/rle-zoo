/*
	Run-Length Encoder, Goldbox Variant
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	See https://github.com/eloj/rle-eddy
*/
#include <stdint.h>
#include <stddef.h>

size_t goldbox_compress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);
size_t goldbox_decompress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);

#ifdef RLE_GOLDBOX_IMPLEMENTATION
#include <assert.h>

// RLE PARAMS: min CPY=1, max CPY=126, min REP=1, max REP=126
size_t goldbox_compress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen) {
	size_t rp = 0;
	size_t wp = 0;

	while (rp < slen && (wp < dlen || dest == NULL)) {
		uint8_t cnt = 0;

		// Count number of same bytes, up to 126
		while ((rp+cnt+1 < slen) && (src[rp+cnt] == src[rp+cnt+1]) && (cnt < 126))
			++cnt;

		// Output REP
		if (cnt > 0 || (rp+cnt+1 == slen)) {
			++cnt;
			if (dest && dlen > 0) {
				dest[wp+0] = -(int8_t)cnt;
				if (wp + 1 < dlen)
					dest[wp+1] = src[rp];
			}
			wp += 2;
			rp += cnt;
			continue;
		}

		cnt = 0;
		while ((rp+cnt+1 < slen) && (src[rp+cnt] != src[rp+cnt+1]) && (cnt < 126)) // Accepting more makes us incompatible with PoR
			++cnt;

		assert(cnt > 0);

		// Output CPY
		if (dest && dlen > 0) {
			dest[wp+0] = cnt - 1;
			// assert(wp + cnt + 1 < dlen);
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

size_t goldbox_decompress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen) {
	const uint8_t *send = src + slen;
	size_t wp = 0;
	while (src < send && (wp < dlen || dest == NULL)) {
		uint8_t cnt;
		uint8_t b = *src++;
		if (b & 0x80) {
			cnt = -(int8_t)b;
			uint8_t out = *src++;
			if (dest && dlen > 0) {
				// memset(dest + wp, out, cnt);
				for (int i = 0 ; i < cnt && wp + i < dlen ; ++i)
					dest[wp + i] = out;
			}
		} else {
			cnt = b + 1;
			if (dest && dlen > 0) {
				// memcpy(dest + wp, src, cnt);
				for (int i = 0 ; i < cnt && wp + i < dlen ; ++i)
					dest[wp + i] = src[i];
			}
			src += cnt;
		}
		wp += cnt;
	}
	assert(src == send);
	assert((dest == NULL) || (wp <= dlen));
	return wp;
}
#endif
