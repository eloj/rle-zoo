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
#include <sys/types.h> // ssize_t

ssize_t pcx_compress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);
ssize_t pcx_decompress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);

#if defined(RLE_ZOO_PCX_IMPLEMENTATION) || defined(RLE_ZOO_IMPLEMENTATION)
#include <assert.h>

static_assert(sizeof(size_t) == sizeof(ssize_t), "");

// return -(rp + 1) ... mask so it can't flip positive. Give up and just always return -1?
#define RLE_ZOO_RETURN_ERR return ~(rp & ((size_t)~0 >> 1UL))

ssize_t pcx_compress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen) {
	size_t rp = 0;
	size_t wp = 0;

	while (rp < slen) {
		assert((ssize_t)wp >= 0);
		assert((ssize_t)rp >= 0);

		size_t cnt = 0;
		// size_t cnt = rle_count_rep(src + rp, slen - rp, 63);
		do { ++cnt; } while ((rp + cnt < slen) && (cnt < 63) && (src[rp + cnt - 1] == src[rp + cnt]));

		// Output REP, also include any bytes that can't be encoded as a LIT.
		if (cnt > 1 || ((src[rp] & 0xC0) == 0xC0)) { // or >= 192
			assert(cnt <= 63);
			if (dest) {
				if (wp + 1 < dlen) {
					dest[wp+0] = (uint8_t)(0xC0 | cnt);
					dest[wp+1] = src[rp];
				} else {
					RLE_ZOO_RETURN_ERR;
				}
			}
			wp += 2;
			rp += cnt;
		} else {
			// Output LIT.
			// PERF: Again, this is probably suboptimal, and also results in encoding runs of 2 LITs as REP, which differs from IM encoder.
			if (dest) {
				if (wp < dlen) {
					dest[wp] = src[rp];
				} else {
					RLE_ZOO_RETURN_ERR;
				}
			}
			++rp;
			++wp;
		}
	}
	assert(rp == slen);
	assert((dest == NULL) || (wp <= dlen));
	return (ssize_t)wp;
}

ssize_t pcx_decompress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen) {
	size_t wp = 0;
	size_t rp = 0;
	while (rp < slen) {
		assert((ssize_t)wp >= 0);
		assert((ssize_t)rp >= 0);

		uint8_t cnt = 1;
		uint8_t b = src[rp++];

		// REP
		if ((b & 0xC0) == 0xC0) {
			if (!(rp < slen)) {
				RLE_ZOO_RETURN_ERR;
			}
			cnt = b & 0x3F;
			b = src[rp++];
		}
		if (dest) {
			if (wp + cnt <= dlen) {
				for (unsigned int i = 0 ; i < cnt ; ++i)
					dest[wp + i] = b;
			} else {
				RLE_ZOO_RETURN_ERR;
			}
		}
		wp += cnt;
	}
	assert(rp == slen);
	assert((dest == NULL) || (wp <= dlen));
	return (ssize_t)wp;
}
#undef RLE_ZOO_RETURN_ERR
#endif

#ifdef __cplusplus
}
#endif
