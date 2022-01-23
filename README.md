
# RLE Zoo

A collection of Run-Length Encoders and Decoders. So far there is only one animal in the zoo. It's a very small zoo.

[![Build status](https://github.com/eloj/rle-zoo/workflows/build/badge.svg)](https://github.com/eloj/rle-zoo/actions/workflows/c-cpp.yml)

## Run-Length Encoding

"_Run-length encoding (RLE) is a form of lossless data compression in which runs of data (sequences in which the same data value occurs in many consecutive data elements) are stored as a single data value and count, rather than as the original run._" -- [Wikipedia](https://en.wikipedia.org/wiki/Run-length_encoding)

At its most basic, a Run-Length Encoder process input into a series of REP (repeat) and CPY (copy) operations.

## Tools

`rle-zoo` can encode and decode files using any of the supplied variants.

`rle-genops` can be used to generate complete code word/OPs lists for supported variants, and contains code that verifies
the encoding and decoding scheme for a variant is consistent. Post-implementation this is mostly useful for debugging,
'manual parsing' and reverse-engineering of unknown RLE streams.

## Variants

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

## TODO

* Add functional conformance tests, e.g verify length determination.
* Add fuzzing (afl++).
* Improve `rle-zoo` to behave more like a standard UNIX filter.

## License

All code is provided under the [MIT License](LICENSE).
