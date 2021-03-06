/*
	Run-Length Encoder/Decoder (RLE), Goldbox Variant
	Copyright (c) 2022, Eddy L O Jansson. Licensed under The MIT License.

	This code has been specifically crafted to be compatible with the SSI Goldbox games.

	See https://github.com/eloj/rle-zoo
*/
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
#include <sys/types.h> // ssize_t
#endif

ssize_t goldbox_compress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);
ssize_t goldbox_decompress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen);

#if defined(RLE_ZOO_GOLDBOX_IMPLEMENTATION) || defined(RLE_ZOO_IMPLEMENTATION)
#include <assert.h>

static_assert(sizeof(size_t) == sizeof(ssize_t), "");

// return -(rp + 1) ... mask so it can't flip positive. Give up and just always return -1?
#define RLE_ZOO_RETURN_ERR return ~(rp & ((size_t)~0 >> 1UL))

// RLE PARAMS: min CPY=1, max CPY=126, min REP=1, max REP=127
ssize_t goldbox_compress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen) {
	size_t rp = 0;
	size_t wp = 0;

	while (rp < slen) {
		assert((ssize_t)wp >= 0);
		assert((ssize_t)rp >= 0);

		uint8_t cnt = 0;

		// Count number of same bytes, up to 126
		while ((rp+cnt+1 < slen) && (src[rp+cnt] == src[rp+cnt+1]) && (cnt < 126)) {
			++cnt;
		}

		// Output REP. Also encode the last characters as a REP, even if it's just one.
		if (cnt > 0 || (rp+cnt+1 == slen)) {
			if (dest) {
				if (wp + 1 < dlen) {
					dest[wp+0] = ~cnt;
					dest[wp+1] = src[rp];
				} else {
					RLE_ZOO_RETURN_ERR;
				}
			}
			wp += 2;
			rp += cnt;
			rp++;
			continue;
		}

		cnt = 0;
		while ((rp+cnt+1 < slen) && (src[rp+cnt] != src[rp+cnt+1]) && (cnt < 126)) { // Accepting more makes us incompatible with PoR
			++cnt;
		}

		assert(cnt > 0);
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

ssize_t goldbox_decompress(const uint8_t *src, size_t slen, uint8_t *dest, size_t dlen) {
	size_t wp = 0;
	size_t rp = 0;
	while (rp < slen) {
		assert((ssize_t)wp >= 0);
		assert((ssize_t)rp >= 0);

		uint8_t cnt;
		uint8_t b = src[rp++];
		if (b & 0x80) {
			// REP
			cnt = (~b) + 1; // equiv. -(int8_t)b
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
