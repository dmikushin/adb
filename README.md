# Android Debug Bridge (ADB) standalone binary for Linux/Windows/MacOS

Android Debug Bridge adaptation that compiles with gcc on Linux, with clang on MacOS, and with Visual Studio on Windows.

## Prerequisites

* Compiler: gcc (Linux), XCode (MacOS), Visual Studio 2019 (Windows)
* CMake, and optionally Ninja for faster build

## Building

Building with default toolchain:

```bash
$ mkdir build && cd build
$ cmake -G Ninja ..
$ ninja
```

