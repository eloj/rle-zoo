/*
	Run-Length Encoder/Decoder (RLE), Apple ICNS Variant
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	See https://github.com/eloj/rle-zoo
*/
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h> // ssize_t

ssize_t icns_compress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);
ssize_t icns_decompress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);

#if defined(RLE_ZOO_ICNS_IMPLEMENTATION) || defined(RLE_ZOO_IMPLEMENTATION)
#include <assert.h>

static_assert(sizeof(size_t) == sizeof(ssize_t), "");

// return -(rp + 1) ... mask so it can't flip positive. Give up and just always return -1?
#define RLE_ZOO_RETURN_ERR return ~(rp & ((size_t)~0 >> 1UL))

// RLE PARAMS: min CPY=1, max CPY=128, min REP=3, max REP=130
ssize_t icns_compress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen) {
	size_t rp = 0;
	size_t wp = 0;

	while (rp < slen) {
		assert((ssize_t)wp >= 0);
		assert((ssize_t)rp >= 0);

		uint8_t cnt = 0;
		do { ++cnt; } while ((rp + cnt < slen) && (cnt < 130) && (src[rp + cnt - 1] == src[rp + cnt]));

		// Output REP.
		if (cnt >= 3) {
			assert(cnt >= 3 && cnt <= 130);
			if (dest) {
				if (wp + 1 < dlen) {
					dest[wp+0] = (uint8_t)(cnt + 125);
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
		int repcnt = 0; // zero-based

		// Count number of literal bytes, up to 128, allowing 2 reps. Ugly.
		do { ++cnt; } while ((rp + cnt < slen) && (cnt < 128) && (
			((src[rp + cnt - 1] != src[rp + cnt]) && !(repcnt = 0)) ||
			(++repcnt < 2)
		));
		// Adjust if over-scanned.
		if (repcnt == 2) {
			cnt -= 2;
		}

		assert(cnt > 0);
		assert(cnt <= 128);
		assert(rp + cnt <= slen);

		// Output CPY
		if (dest) {
			if (wp + cnt + 1 <= dlen) {
				dest[wp] = cnt - 1;
				for (unsigned int i = 0 ; i < cnt ; ++i)
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
	return (ssize_t)wp;
}

ssize_t icns_decompress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen) {
	size_t wp = 0;
	size_t rp = 0;
	while (rp < slen) {
		assert((ssize_t)wp >= 0);
		assert((ssize_t)rp >= 0);

		uint8_t cnt = 0;
		uint8_t b = src[rp++];
		if (b >= 0x80) {
			// REP
			cnt = (uint8_t)(b - 125);
			if (!(rp < slen)) {
				RLE_ZOO_RETURN_ERR;
			}
			if (dest) {
				if (wp + cnt <= dlen) {
					for (unsigned int i = 0 ; i < cnt ; ++i)
						dest[wp + i] = src[rp];
				} else {
					RLE_ZOO_RETURN_ERR;
				}
			}
			++rp;
		} else {
			// CPY
			cnt = b + 1;
			if (!(rp + cnt <= slen)) {
				RLE_ZOO_RETURN_ERR;
			}
			if (dest) {
				if (wp + cnt <= dlen) {
					for (unsigned int i = 0 ; i < cnt ; ++i)
						dest[wp + i] = src[rp + i];
				} else {
					RLE_ZOO_RETURN_ERR;
				}
			}
			rp += cnt;
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
