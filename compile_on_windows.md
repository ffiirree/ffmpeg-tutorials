# Windows & MSYS2

## CMake

Download form [CMake](https://cmake.org/download/) and install on **Windows**.

## MSYS2

### Installation

Download from [MSYS2](https://www.msys2.org/) and install.

### ASM

```bash
pacman -S yasm nasm
```

### MSYS2 `<path_to>\msys64\msys2_shell.cmd`

Change `rem set MSYS2_PATH_TYPE=inherit` to `set MSYS2_PATH_TYPE=inherit`

## libx264

### download

```bash
git clone https://code.videolan.org/videolan/x264.git
```

### config & compile

```bash
cd x264
./configure --prefix=/usr/local --enable-static
make -j 8
make install
```

## libx265

### download

```bash
git clone https://bitbucket.org/multicoreware/x265_git.git
```

### config & compile

```bash
cd x265_git/build
cmake -G "MSYS Makefiles" ../source -DCMAKE_INSTALL_PREFIX=/usr/local -DENABLE_SHARED=OFF -DCMAKE_EXE_LINKER_FLAGS="-static"

make -j 8
make install
```

## FFmpeg

### download

Download from [FFmpeg](https://www.ffmpeg.org/download.html#releases).

### config & compile

```bash
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
export LDFLAGS="-static-libgcc -static-libstdc++ "

cd ffmpeg-4.4.2
./configure --prefix=ffmpeg_build \
    --enable-gpl --enable-version3 \
    --enable-libx264 \
    --enable-libx265 \
    --disable-w32threads \
    --enable-shared  \
    --extra-ldflags=-static --pkg-config-flags=--static

make -j 8
make install
```
