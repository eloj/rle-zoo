
# RLE Zoo

A collection of Run-Length Encoders and Decoders. So far there is only one animal in the zoo. It's a very small zoo.

[![Build status](https://github.com/eloj/rle-eddy/workflows/build/badge.svg)](https://github.com/eloj/rle-eddy/actions/workflows/c-cpp.yml)

## Goldbox

The `goldbox` variant has been organically hand-crafted to be 100% compatible with the classic Goldbox games _Pool of Radiance_ and _Curse of the Azure Bonds_,
and probably more. I have round-trip verified the decompressor and compressor on all resource files in both of those games, though the tests provided
in this repository are much more modest.

This variant is mostly interesting in the ways it's suboptimal; It's slightly short-stroked, and doesn't deal optimally with
runs at the end of the input.

### Format

* One byte encoding the `length` of the following run.
* If the high-bit is NOT set (or equally; signed `length` is non-negative), then `length + 1` literal bytes follows (CPY)
* If the high-bit IS set (or equally; signed `length` is negative), then the next byte is to be repeated the absolute value of the signed `length` times (REP)
* ... repeat ...

The encoder will never output a literal run longer than 126 characters, or a run of repeated characters longer than 127. It will also
insist on encoding the last part of the file as a REP if preceeded by a CPY.

To the last point, the input `1234` will be encoded as `02 31 32 33 ff 34`, e.g CPY 3 characters, then REP 1 character.
An input that ends in a run, like `AAAA` will encode as expected: `fc 41`

## License

All code is provided under the [MIT License](LICENSE).
