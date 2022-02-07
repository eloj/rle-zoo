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
#include <sys/types.h> // ssize_t

ssize_t packbits_compress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);
ssize_t packbits_decompress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);

#ifdef RLE_ZOO_PACKBITS_IMPLEMENTATION
#include <assert.h>

#define RLE_ZOO_RETURN_ERR return ~rp

// RLE PARAMS: min CPY=1, max CPY=128, min REP=2, max REP=128
ssize_t packbits_compress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen) {
	size_t rp = 0;
	size_t wp = 0;

	while (rp < slen) {
		uint8_t cnt = 0;
		do { ++cnt; } while ((rp + cnt + 1 <= rp + slen) && (cnt < 128) && (src[rp + cnt-1] == src[rp + cnt]));

		// Output REP.
		if (cnt > 1) {
			assert(cnt >= 2 && cnt <= 128);
			if (dest) {
				if (wp + 1 < dlen) {
					dest[wp+0] = 257 - cnt;
					dest[wp+1] = src[rp];
				} else {
					RLE_ZOO_RETURN_ERR;
				}
			}
			wp += 2;
			rp += cnt;
			continue;
		}

		cnt = 0;
		// Count number of literal bytes, up to 128.
		while ((rp+cnt+1 <= slen) && (cnt < 128) && ((rp+cnt+1 == slen) || (src[rp+cnt] != src[rp+cnt+1]))) {
			++cnt;
		}

		assert(cnt > 0);
		assert(cnt <= 128);
		assert(rp + cnt <= slen);

		// Output CPY
		if (dest) {
			if (wp + cnt + 1 <= dlen) {
				dest[wp] = cnt - 1;
				for (int i = 0 ; i < cnt ; ++i)
					dest[wp + 1 + i] = src[rp + i];
			} else {
				RLE_ZOO_RETURN_ERR;
			}
		}
		rp += cnt;
		wp += cnt + 1;
	}
	assert(rp == slen);
	assert((dest == NULL) || (wp <= dlen));
	return wp;
}

ssize_t packbits_decompress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen) {
	size_t wp = 0;
	size_t rp = 0;
	while (rp < slen) {
		uint8_t cnt = 0;
		uint8_t b = src[rp++];
		if (b > 0x80) {
			// REP
			cnt = 257 - b;
			if (!(rp < slen)) {
				RLE_ZOO_RETURN_ERR;
			}
			if (dest) {
				if (wp + cnt <= dlen) {
					for (int i = 0 ; i < cnt ; ++i)
						dest[wp + i] = src[rp];
				} else {
					RLE_ZOO_RETURN_ERR;
				}
			}
			++rp;
		} else if (b < 0x80) {
			// CPY
			cnt = b + 1;
			if (!(rp + cnt <= slen)) {
				RLE_ZOO_RETURN_ERR;
			}
			if (dest) {
				if (wp + cnt <= dlen) {
					for (int i = 0 ; i < cnt ; ++i)
						dest[wp + i] = src[rp + i];
				} else {
					RLE_ZOO_RETURN_ERR;
				}
			}
			rp += cnt;
		} // else b == 0x80: Reserved. Just skip byte as suggested by TN1023.
		wp += cnt;
	}
	assert(rp == slen);
	assert((dest == NULL) || (wp <= dlen));
	return wp;
}
#undef RLE_ZOO_RETURN_ERR
#endif

#ifdef __cplusplus
}
#endif
