# BigIntC

![Code Coverage](https://github.com/Totto16/bigint_c/blob/_xml_coverage_reports/data/main/badge.svg)

## What is this?

This is a cross-platform BigInt library written in C with Builtin C++ support. (not just wrappers, but operator overloading etc.)

## Platform support

This officially supports Linux, Windows and MacOS.

## Features

### General

- Construct arbitrary large integers (both positive and negative numbers) in C from a string
- C++ Wraps it in a struct, that is move only, it can be copied, but you need to do it explicitly, via `big_int.copy()`
- C++ constexpr literal, just write `"1141414141"_n` with arbitrary large strings (you may need to increase the constexpr steps for large strings, see the tests cases on how to do that!)
- The C++ Struct has overloaded operators for all supported arithmetic instructions (e.g. `+`)
- The C++ Struct supports implicit conversions from uin64_t and int64_t.
- Supports serialization to string in dec, bin and hex
- Completely tested, by comparing computation results to the `gmp` or `tommath` library
- The C++ library is header only and in the style of stb libraries, see tests on how to use it, it is completely optional and no C++ compiler is needed, if you only use the C library.
- This implementation stores the underlying data memory efficiently as an uint64_t array
- supports C++ hash, it can be used as index in hashmaps and similar data structures
- supports stream output with io modifiers like `std::bin` and similar

### Operations

#### Basic Operations

- [x] Addition
- [x] Subtraction
- [x] Negation
- [x] Multiplication
- [ ] Division
- [ ] Modulo
- [ ] Exponentiation

#### Bitwise Operations

- [ ] Shift (right + left)
- [ ] Xor
- [ ] Or
- [ ] And
- [ ] Complement (~)

#### Other Operations

- [x] Comparison
