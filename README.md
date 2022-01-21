
# RLE Zoo

A collection of Run-Length Encoders and Decoders. So far there is only one animal in the zoo. It's a very small zoo.

[![Build status](https://github.com/eloj/rle-eddy/workflows/build/badge.svg)](https://github.com/eloj/rle-eddy/actions/workflows/c-cpp.yml)

## Goldbox

The `goldbox` variant has been crafted to be 100% compatible with the classic Goldbox games _Pool of Radiance_ and _Curse of the Azure Bonds_,
and probably more. I have round-trip verified the decompressor and compressor on all resource files in both of those games, though the tests provided
in this repository are much more modest.

This variant is mostly interesting in the ways it's suboptimal; It's slightly short-stroked, and doesn't deal optimally with
runs at the end of the input.

## License

All code is provided under the [MIT License](LICENSE).
