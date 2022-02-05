
# RLE Zoo

[![Build status](https://github.com/eloj/rle-zoo/workflows/build/badge.svg)](https://github.com/eloj/rle-zoo/actions/workflows/c-cpp.yml)

A collection of Run-Length Encoders and Decoders, and associated tooling for exploring this space. So far there are only two animals in the zoo. It's a very small zoo.

* _WHILE THIS NOTE PERSISTS, I MAY FORCE PUSH TO MASTER_
* The codecs are written foremost to be clear and easy to understand, not for speed.
* I'm mostly interested in exploring legacy formats.

## What is Run-Length Encoding?

"_Run-length encoding (RLE) is a form of lossless data compression in which runs of data (sequences in which the same data value occurs in many consecutive data elements) are stored as a single data value and count, rather than as the original run._" -- [Wikipedia](https://en.wikipedia.org/wiki/Run-length_encoding)

At its most basic, a Run-Length Encoder process input into a series of REP (repeat) and CPY (copy) operations.
Sometimes the CPY operation is explicit, sometimes it's in the form of literals (LIT).

## I just need one, what do I do?

The `rle_*.h` files are single-header libraries. If you just need one, any one, I recommend downloading `rle_packbits.h` and
looking at `test_example.c` for how to use it.

```c
#define RLE_ZOO_PACKBITS_IMPLEMENTATION
#include "rle_packbits.h"

int main(void) {
	const uint8_t input[] = "ABBBBA";
	size_t len = sizeof(input) - 1;

	// Call with NULL for dest buffer and size to calculate output size.
	size_t expected_size = packbits_compress(input, len, NULL, 0);
	...
```

## Tools

`rle-zoo` can encode and decode files using any of the supplied variants.

`rle-genops` can be used to generate complete code word/OPs lists for supported variants, and contains code that verifies
the encoding and decoding scheme for a variant is consistent. Post-implementation this is mostly useful for debugging,
'manual parsing' and reverse-engineering of unknown RLE streams. It can also generate C tables for implementing table-driven
encoders and decoders.

`rle-parser` can be used to parse a file using the available RLE variants, which could help identify the
variant used on some unknown data. It also acts as a demonstrator for using `rle-genops` tables.

## Zoo Animals

Currently the following extraordinary specimens are grazing the fertile grounds of this most amazing Zoo:

* Packbits
* Goldbox
* PCX

### PackBits

The `packbits` variant reportedly originates from the Apple Macintosh[^fnTN1023], but spread far and wide from there, including
into Electronic Arts IFF ILBM (unverified).

#### PackBits Format

* One OP byte encoding the operation and `length`:
	* 0x00 => CPY 1
	* 0x01 => CPY 2
	* ..
	* 0x7e => CPY 127
	* 0x7f => CPY 128
	* 0x80 => _reserved_
	* 0x81 => REP 128
	* 0x82 => REP 127
	* ..
	* 0xfe => REP 3
	* 0xff => REP 2

* CPY: If OP high-bit is CLEAR (equally; signed OP is non-negative), then `OP + 1` literal bytes follows.
* REP: If OP high-bit is SET (equally; signed OP is negative), the next byte is repeated 257 - OP (or 1 - signed OP) times.

The encoder should not generate 0x80, which is reserved. The technical note from Apple states that a decoder should
"_[handle] this situation by skipping any flag-counter byte with this value and interpreting the next byte as the next flag-counter byte._"

[^fnTN1023]: "Understanding PackBits", Apple [Technical Note TN1023](http://web.archive.org/web/20080705155158/http://developer.apple.com/technotes/tn/tn1023.html).

### Goldbox

The `goldbox` variant has been organically hand-crafted to be 100% compatible with the classic Goldbox games _Pool of Radiance_ and _Curse of the Azure Bonds_,
and probably more. I have round-trip verified the decompressor and compressor on all resource files in both of those games, though the tests provided
in this repository are much more modest.

This variant is mostly interesting in the ways it's suboptimal; It's slightly short-stroked, and doesn't deal optimally with the end of the input.

#### Goldbox Format

* One OP byte encoding the operation and `length`:
	* 0x00 => CPY 1
	* 0x01 => CPY 2
	* ..
	* 0x7d => CPY 126
	* 0x7e => CPY 127 ; _not used by encoder_
	* 0x7f => CPY 128 ; _not used by encoder_
	* 0x80 => REP 128 ; _not used by encoder_
	* 0x81 => REP 127
	* 0x82 => REP 126
	* ..
	* 0xfe => REP 2
	* 0xff => REP 1
* CPY: If OP high-bit is CLEAR (equally; signed OP is non-negative), then `OP + 1` literal bytes follows.
* REP: If OP high-bit is SET (equally; signed OP is negative), then the next byte is to be repeated 1 + the bitwise complement of OP (or equally; the negation of the signed `OP`) times.

To the best of my knowledge, the encoder will never use OPs 0x7e, 0x7f or 0x80, i.e it will never output a literal run of 127 or 128 characters, nor a run of 128 repeated characters.

In addition to not using all the encodings, having both CPY 1 and REP 1 is obviously suboptimal.

It will also insist on encoding the tail of the input as a REP OP if it hits the end while counting literals to CPY.

To the last point, the input `1234` will be encoded as `02 31 32 33 ff 34`, i.e CPY 3 characters, then REP 1 character.

These limits and quirks were determined experimentally using existing game data files. The decoders in the games MAY accept
more optimal encodings, but the encoder provided here was specifically crafted such that encoding the output of the decoder
is bit-identitical to the original input.

### PCX

The `PCX` variant comes from the _ZSoft IBM PC Paintbrush_ software and its associated [PCX image format](https://en.wikipedia.org/wiki/PCX).
This image format was somewhat popular on the PC platform in the mid 1980s up to the early 1990s, but is now thoroughly obsolete.

The only compression ever defined for this format is a simple RLE variant which uses two bits of a byte to encode
REPs, and leaves the rest as literals.

#### PCX Format

* One OP byte encoding REP and `length`, or used as-is:
	* 0x00 => LIT 0
	* 0x01 => LIT 1
	* ..
	* 0xbd => LIT 189
	* 0xbe => LIT 190
	* 0xbf => LIT 191
	* 0xc0 => REP 0
	* 0xc1 => REP 1
	* 0xc2 => REP 2
	* ..
	* 0xfe => REP 62
	* 0xff => REP 63
* REP: If the top two bits of OP are SET (equally; OP is >= 192), then the next byte is repeated `OP & 0x3F` times (0-63).
* LIT: Any other byte value is used as-is (0-191).

Obviously any single input byte larger than 191 MUST be encoded as a REP 1 -- expanding the output with one byte -- but an
encoder COULD encode any value using REP 1. In addition, depending on how the encoder is structured, it may encode two repeated
literals as-is, or as a REP 2. An encoder that scans for runs of literals will probably do the first, while one that output
literals as they are seen will probably do the former.

The existance of a `REP 0` operation is an inefficiency, and allows the encoder to encode data that is not included in the decoded output.

## TODO

* Add functional conformance tests, e.g verify length determination.
* Add more animals. Potential candidates: Apple 'icns' Icons, BMP(?), TGA, ...
* Add fuzzing (afl++).
* Improve `rle-zoo` to behave more like a standard UNIX filter.

## License

All code is provided under the [MIT License](LICENSE).
