# xxHash

`xxhash.h` from xxHash v0.8.3:
https://github.com/Cyan4973/xxHash/blob/v0.8.3/xxhash.h

BSD-2-Clause license and copyright are included in the header.
This vendored file keeps upstream formatting, with trailing whitespace removed.

Generics compiles private inline copies, without exporting xxHash C symbols.
The baseline and separately compiled AVX2 / AVX-512 implementations produce
identical XXH3-64 results. Runtime selection uses Generics::Simd capabilities.
The public C++ wrapper stores the pinned version's state inline; compile-time
checks detect incompatible size or alignment changes when upgrading xxHash.
