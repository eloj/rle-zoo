
# RLE Zoo Fuzzing Driver

This directory contains a short driver program optimized for running the
compression and decompression code through a fuzzer, specifically [AFL++ - American Fuzzy Lop++](https://aflplus.plus/).

It's written to run in the persistent mode with shared memory, which is
the optimal mode for speed, but requires LLVM mode.

## Prerequisites

You need to install AFL++, specifically you need `afl-clang-fast`.

## Building & Running

Running `make afl-driver` will build the default driver using `afl-clang-fast`. You
can overrider the AFL++ wrapper by setting e.g `AFLCC=afl-cc`.

You can `make fuzz` or `make fuzz-driver` to build the a driver and start AFL++.

NOTE: It's recommended to run fuzzing on /tmp (or another RAM disk), since AFL++ can generate a lot of disk writes.

## Debugging crashes

Verify any crashes by passing the supposed crash input into `rle-zoo`, or the `afl-driver` itself, e.g:

```bash
$ ./afl-driver < findings/default/crashes/id:000000,sig:06,src:000007,time:6,execs:486,op:havoc,rep:8
afl-driver: ./rle_packbits.h:77: ssize_t packbits_compress(const uint8_t *, size_t, uint8_t *, size_t): Assertion `rp == slen' failed.
Aborted
```

Once verified, the relevant function can be fixed and the input added to the tests.
