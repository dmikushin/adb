# Android Debug Bridge (ADB) standalone binary for Linux/Windows/MacoS

## Prerequisites

* For arm/arm64 Linux cross-compilation on Linux you may need to install GCC toolchain, e.g. from [here](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads)

```bash
$ cd cmake/toolchain/linux-aarch64
$ wget 'https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz\?revision\=61c3be5d-5175-4db6-9030-b565aae9f766\&la\=en\&hash\=0A37024B42028A9616F56A51C2D20755C5EBBCD7' -O gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz
$ tar xvf gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz --strip-components=1
$ cd -
```

* For compilation with GCC toolchain on Windows you can use MSYS2:

```
$ pacman -S mingw-w64-i686-gcc
$ pacman -S mingw-w64-i686-cmake
$ pacman -S make
```

MSYS2 mirror for China: https://mirror.tuna.tsinghua.edu.cn/help/msys2/

## Building

Building with default toolchain:

```bash
$ mkdir build && cd build
$ cmake -G Ninja ..
$ ninja
```

Building with cross-compilation toolchain:

```
$ CC=clang CXX=clang++ cmake . -Bbuild-arm64 -DCMAKE_TOOLCHAIN_FILE=cmake/linux/toolchain-aarch64.cmake
$ cmake --build build-arm64 --config Release
```

See further info here: https://clickhouse.tech/docs/en/development/build-cross-arm/

## Troubleshooting

