# Windows & MSYS2

## MSYS2

1. Download [MSYS2](https://www.msys2.org/) and install it.
2. Edit `C:/msys64/msys2_shell.cmd` and remove `rem` from the line with `rem MSYS2_PATH_TYPE=inherit`
3. Open a x64 Native Tools Command Prompt for VS 2022
4. Run `C:/msys64/msys2_shell.cmd`
5. Run:

```bash
pacman -Syu
pacman -S make
pacman -S diffutils
pacman -S pkg-config
pacman -S yasm
pacman -S nasm

# check compiler
# It should be some one likes `/c/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.36.32532/bin/HostX64/x64/cl`
which cl

# check linker
# It should be some one likes `/c/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.36.32532/bin/HostX64/x64/link`
which link

# hide the /usr/bin/link
mv /usr/bin/link.exe /usr/bin/link.exe.bak
```

## Sources

```bash
mkdir sources && mkdir ffmpeg_shared && cd sources

# FFmpeg
git clone https://github.com/FFmpeg/FFmpeg.git
# x264
git clone https://code.videolan.org/videolan/x264.git
```

## Compile libx264

```bash
cd x264
CC=cl ./configure --prefix=/usr/local --enable-static --disable-cli
make -j 16
make install
```

## Compile FFmpeg with libx264

```bash
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

CC=cl ~/sources/FFmpeg/configure --prefix=../../ffmpeg_shared \
    --target-os=win64 \
    --toolchain=msvc \
    --arch=x86_64 \
    --enable-x86asm \
    --enable-asm \
    --enable-shared \
    --enable-gpl --enable-version3 \
    --enable-libx264


make -j 16
make install
```

## Crop FFmpeg

| Format                               | Type                                     | Option                                                                              | Repository                                                                      |
| ------------------------------------ | ---------------------------------------- | ----------------------------------------------------------------------------------- | ------------------------------------------------------------------------------- |
| H.264 / AVC                          | Video / Encoder /                        | --enable-libx264                                                                    | [libx264](https://code.videolan.org/videolan/x264.git)                          |
| H.265 / HEVC                         | Video / Encoder /                        | --enable-libx265                                                                    | [libx265](https://bitbucket.org/multicoreware/x265_git.git)                     |
| VP8 / VP9                            | Video                                    | --enable-libvpx                                                                     | [libvpx](https://chromium.googlesource.com/webm/libvpx)                         |
| AV1                                  | Video / Encoder / Decoder                | --enable-libaom                                                                     | [libaom](https://aomedia.googlesource.com/aom)                                  |
| AV1                                  | Video /         / Decoder                | --enable-libdav1d                                                                   | [dav1d](https://code.videolan.org/videolan/dav1d.git)                           |
| AV1                                  | Video / Encoder /                        | --enable-libsvtav1                                                                  | [SVT-AV1](https://gitlab.com/AOMediaCodec/SVT-AV1.git)                          |
| MPEG-4 Part 2 / MPEG-4               | Video / Encoder /                        | --enable-libxvid                                                                    | [libxvid](https://labs.xvid.com/source/)                                        |
| Theora video compression             | Video / Encoder /                        | --enable-libtheora                                                                  | [libtheora](https://www.theora.org/downloads/)                                  |
|                                      |                                          |                                                                                     |                                                                                 |
| Video Multi-Method Assessment Fusion | Video / Filter  /                        | --enable-libvmaf                                                                    | [libvmaf](https://github.com/Netflix/vmaf.git)                                  |
| Video Stabilization                  | Video                                    | --enable-libvidstab                                                                 | [libvidstab](https://github.com/georgmartius/vid.stab.git)                      |
|                                      |                                          |                                                                                     |                                                                                 |
| MP3                                  | Audio / Encoder /                        | --enable-libmp3lame                                                                 | [libmp3lame](https://lame.sourceforge.io/)                                      |
| AAC                                  | Audio / Encoder / Decoder                | --enable-encoder=aac <br> --enable-decoder=aac                                      | built-in                                                                        |
| AAC                                  | Audio / Encoder / Decoder                | --enable-libfdk-aac                                                                 | [libfdk-aac](https://git.code.sf.net/p/opencore-amr/fdk-aac)                    |
| Opus                                 | Audio / Encoder / Decoder                | -–enable-libopus                                                                    | [libopus](https://github.com/xiph/opus.git)                                     |
| GSM 06.10                            | Audio / Encoder / Decoder                | --enable-libgsm                                                                     | [libgsm](https://quut.com/gsm/)                                                 |
| Adaptive Multi Rate (AMR-NB)         | Audio / Encoder / Decoder                | --enable-libopencore-amrnb                                                          | [libopencore-amr](https://github.com/BelledonneCommunications/opencore-amr.git) |
| Adaptive Multi Rate (AMR-WB)         | Audio /         / Decoder                | --enable-libopencore-amrwb                                                          | [libopencore-amr](https://github.com/BelledonneCommunications/opencore-amr.git) |
| OpenMPT                              | Audio /         / Decoder                | --enable-libopenmpt                                                                 | [libopenmpt](https://lib.openmpt.org/libopenmpt/)                               |
| Vorbis                               | Audio / Encoder / Decoder                | --enable-libvorbis                                                                  | [libvorbis](https://gitlab.xiph.org/xiph/vorbis.git)                            |
| Game Music Emu                       | Audio                                    | --enable-libgme                                                                     | [libgme](https://github.com/mcfiredrill/libgme)                                 |
| RubberBand                           | Audio / Filter                           | --enable-librubberband                                                              | [librubberband](https://github.com/breakfastquay/rubberband.git)                |
|                                      |                                          |                                                                                     |                                                                                 |
| zimg                                 | Image                                    | --enable-libzimg                                                                    | [libzimg](https://github.com/sekrit-twc/zimg.git)                               |
|                                      |                                          |                                                                                     |                                                                                 |
| Freetype                             | Text                                     | --enable-libfreetype                                                                | [libfreetype](https://download.savannah.gnu.org/releases/freetype/)             |
|                                      |                                          |                                                                                     |                                                                                 |
| ASS / SSA                            | Subtitle                                 | --enable-libass                                                                     | [libass](https://github.com/libass/libass.git)                                  |
|                                      |                                          |                                                                                     |                                                                                 |
| Unicode Bidirectional Algorithm      | Algorithm                                | --enable-libfribidi                                                                 | [libfribidi](https://github.com/fribidi/fribidi.git)                            |
|                                      |                                          |                                                                                     |                                                                                 |
| Secure Reliable Transport            | Protocol                                 | --enable-libsrt                                                                     | [libsrt](https://github.com/hwangsaeul/libsrt.git)                              |
| SSH                                  | Protocol                                 | --enable-libssh                                                                     | [libssh](https://github.com/CanonicalLtd/libssh.git)                            |
| ZeroMQ                               | Protocol                                 | --enable-libzmq                                                                     | [libzmq](https://github.com/zeromq/libzmq.git)                                  |
|                                      |                                          |                                                                                     |                                                                                 |
| AMF                                  | HWAccel / AMD        / Encoder /         |                                                                                     |                                                                                 |
| NVENC / NVDEC / CUVID                | HWAccel / NVIDIA     / Encoder / Decoder | --disable-nvenc [autodetect] <br> --disable-nvdec <br> --disable-cuvid [autodetect] | [nv-codec-headers](https://git.videolan.org/git/ffmpeg/nv-codec-headers.git)    |
| Direct3D 11                          | HWAccel / Microsoft  /         / Decoder | --disable-d3d11va [autodetect]                                                      |                                                                                 |
| Direct3D 9 / DXVA2                   | HWAccel / Microsoft  /         / Decoder | --disable-dxva2 [autodetect]                                                        |                                                                                 |
| Intel MediaSDK                       | HWAccel / Intel      / Encoder / Decoder | --enable-libmfx                                                                     | [MediaSDK](https://github.com/Intel-Media-SDK/MediaSDK.git)                     |
| oneVPL                               | HWAccel / Intel      / Encoder / Decoder | --enable-libvpl                                                                     | [oneVPL](https://github.com/oneapi-src/oneVPL.git)                              |
| Vulkan                               | HWAccel /            /         / Decoder | --disable-vulkan [autodetect]                                                       |                                                                                 |
| Media Foundation                     |                                          | --enable-mediafoundation [autodetect]                                               |                                                                                 |

## Libraries Size

| Component        | Basic (MB) | Following Build (MB) | Full Build (MB) |
| ---------------- | ---------: | -------------------: | --------------: |
| avcodec-60.dll   |       12.0 |                 19.9 |            73.7 |
| avdevice-60.dll  |        0.2 |                  0.2 |             3.7 |
| avfilter-9.dll   |        4.0 |                  5.9 |            37.7 |
| avformat-60.dll  |        2.3 |                  2.3 |            16.0 |
| avutil-58.dll    |        1.0 |                  1.0 |             2.1 |
| postproc-57.dll  |        0.1 |                  0.1 |             0.1 |
| swresample-4.dll |        0.2 |                  0.2 |             0.4 |
| swscale-7.dll    |        0.7 |                  0.7 |             0.6 |
| total            |       20.5 |                 30.4 |           134.4 |


## Preliminaries

- How to check the compiled lib with pkg-config: `pkg-config --exists --print-errors <lib>`
- How to check the error while compiling ffmpeg: `cat ffbuild/config.log`

## H.265 / libx265

- Install CMake on your system.
- Then

```bash
git clone https://bitbucket.org/multicoreware/x265_git.git

cd x265_git && mkdir cmake-build && cd cmake-build

cmake ../source -DCMAKE_INSTALL_PREFIX=/usr/local -DHIGH_BIT_DEPTH=ON -DENABLE_SHARED=OFF -DENABLE_CLI=OFF -DSTATIC_LINK_CRT=ON
cmake --build . --config Release --target install

# if pkg-config can not find the x265
#   1. edit '/usr/local/lib/pkgconfig/x265.pc' and remove 'C:/msys64' from the line 'prefix=C:/msys64/usr/local'
#   2. rename the x265-static.lib
mv /usr/local/lib/x265-static.lib /usr/local/lib/x265.lib
```

## NVIDIA GPU Hardware Acceleration

- Install CUDA toolkit on your system.
- Clone & Install ffnvcodec

```bash
git clone https://git.videolan.org/git/ffmpeg/nv-codec-headers.git
cd nv-codec-headers
make install

mkdir ~/sources/cuda && cd ~/sources/cuda
cp -r /c/Program\ Files/NVIDIA\ GPU\ Computing\ Toolkit/CUDA/v12.1/include/ .
cp -r /c/Program\ Files/NVIDIA\ GPU\ Computing\ Toolkit/CUDA/v12.1/lib/x64/ .
```

- FFmpeg will autodetect the NVENC / NVDEC / CUVID

## FFmpeg

```bash
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

cd ~/sources/FFmpeg

# + libx264
# + libx265
# + cuda
# + nvenc
# + nvdec
# + libnpp
# + mediafoundation
# + d3d11va
# + dxva2
# - vfwcap
CC=cl ~/sources/FFmpeg/configure --prefix=../../ffmpeg_shared \
    --target-os=win64 \
    --toolchain=msvc \
    --arch=x86_64 \
    --enable-x86asm \
    --enable-asm \
    --enable-shared \
    --enable-gpl --enable-version3 --enable-nonfree \
    --disable-debug \
    --disable-doc \
    --disable-programs \
    --disable-indev=vfwcap \
    --enable-libx264 \
    --enable-libx265 \
    --enable-cuda-nvcc \
    --enable-libnpp \
    --extra-cflags=-I../cuda/include/ \
    --extra-ldflags=-libpath:../cuda/x64/ \
    --extra-version="cropping_build-ffiirree"


make -j 16
make install
```

### References

1. [Compiling FFmpeg with X264 on Windows 10 using MSVC](https://www.roxlu.com/2019/062/compiling-ffmpeg-with-x264-on-windows-10-using-msvc)
2. [Using FFmpeg with NVIDIA GPU Hardware Acceleration](https://docs.nvidia.com/video-technologies/video-codec-sdk/11.1/ffmpeg-with-nvidia-gpu/index.html)
3. [m-ab-s/media-autobuild_suite](https://github.com/m-ab-s/media-autobuild_suite)
4. FFmpeg 6.0 Options

```bash
Usage: configure [options]
Options: [defaults in brackets after descriptions]

Help options:
  --help                   print this message
  --quiet                  Suppress showing informative output
  --list-decoders          show all available decoders
  --list-encoders          show all available encoders
  --list-hwaccels          show all available hardware accelerators
  --list-demuxers          show all available demuxers
  --list-muxers            show all available muxers
  --list-parsers           show all available parsers
  --list-protocols         show all available protocols
  --list-bsfs              show all available bitstream filters
  --list-indevs            show all available input devices
  --list-outdevs           show all available output devices
  --list-filters           show all available filters

Standard options:
  --logfile=FILE           log tests and output to FILE [ffbuild/config.log]
  --disable-logging        do not log configure debug information
  --fatal-warnings         fail if any configure warning is generated
  --prefix=PREFIX          install in PREFIX [/usr/local]
  --bindir=DIR             install binaries in DIR [PREFIX/bin]
  --datadir=DIR            install data files in DIR [PREFIX/share/ffmpeg]
  --docdir=DIR             install documentation in DIR [PREFIX/share/doc/ffmpeg]
  --libdir=DIR             install libs in DIR [PREFIX/lib]
  --shlibdir=DIR           install shared libs in DIR [LIBDIR]
  --incdir=DIR             install includes in DIR [PREFIX/include]
  --mandir=DIR             install man page in DIR [PREFIX/share/man]
  --pkgconfigdir=DIR       install pkg-config files in DIR [LIBDIR/pkgconfig]
  --enable-rpath           use rpath to allow installing libraries in paths
                           not part of the dynamic linker search path
                           use rpath when linking programs (USE WITH CARE)
  --install-name-dir=DIR   Darwin directory name for installed targets

Licensing options:
  --enable-gpl             allow use of GPL code, the resulting libs
                           and binaries will be under GPL [no]
  --enable-version3        upgrade (L)GPL to version 3 [no]
  --enable-nonfree         allow use of nonfree code, the resulting libs
                           and binaries will be unredistributable [no]

Configuration options:
  --disable-static         do not build static libraries [no]
  --enable-shared          build shared libraries [no]
  --enable-small           optimize for size instead of speed
  --disable-runtime-cpudetect disable detecting CPU capabilities at runtime (smaller binary)
  --enable-gray            enable full grayscale support (slower color)
  --disable-swscale-alpha  disable alpha channel support in swscale
  --disable-all            disable building components, libraries and programs
  --disable-autodetect     disable automatically detected external libraries [no]

Program options:
  --disable-programs       do not build command line programs
  --disable-ffmpeg         disable ffmpeg build
  --disable-ffplay         disable ffplay build
  --disable-ffprobe        disable ffprobe build

Documentation options:
  --disable-doc            do not build documentation
  --disable-htmlpages      do not build HTML documentation pages
  --disable-manpages       do not build man documentation pages
  --disable-podpages       do not build POD documentation pages
  --disable-txtpages       do not build text documentation pages

Component options:
  --disable-avdevice       disable libavdevice build
  --disable-avcodec        disable libavcodec build
  --disable-avformat       disable libavformat build
  --disable-swresample     disable libswresample build
  --disable-swscale        disable libswscale build
  --disable-postproc       disable libpostproc build
  --disable-avfilter       disable libavfilter build
  --disable-pthreads       disable pthreads [autodetect]
  --disable-w32threads     disable Win32 threads [autodetect]
  --disable-os2threads     disable OS/2 threads [autodetect]
  --disable-network        disable network support [no]
  --disable-dct            disable DCT code
  --disable-dwt            disable DWT code
  --disable-error-resilience disable error resilience code
  --disable-lsp            disable LSP code
  --disable-mdct           disable MDCT code
  --disable-rdft           disable RDFT code
  --disable-fft            disable FFT code
  --disable-faan           disable floating point AAN (I)DCT code
  --disable-pixelutils     disable pixel utils in libavutil

Individual component options:
  --disable-everything     disable all components listed below
  --disable-encoder=NAME   disable encoder NAME
  --enable-encoder=NAME    enable encoder NAME
  --disable-encoders       disable all encoders
  --disable-decoder=NAME   disable decoder NAME
  --enable-decoder=NAME    enable decoder NAME
  --disable-decoders       disable all decoders
  --disable-hwaccel=NAME   disable hwaccel NAME
  --enable-hwaccel=NAME    enable hwaccel NAME
  --disable-hwaccels       disable all hwaccels
  --disable-muxer=NAME     disable muxer NAME
  --enable-muxer=NAME      enable muxer NAME
  --disable-muxers         disable all muxers
  --disable-demuxer=NAME   disable demuxer NAME
  --enable-demuxer=NAME    enable demuxer NAME
  --disable-demuxers       disable all demuxers
  --enable-parser=NAME     enable parser NAME
  --disable-parser=NAME    disable parser NAME
  --disable-parsers        disable all parsers
  --enable-bsf=NAME        enable bitstream filter NAME
  --disable-bsf=NAME       disable bitstream filter NAME
  --disable-bsfs           disable all bitstream filters
  --enable-protocol=NAME   enable protocol NAME
  --disable-protocol=NAME  disable protocol NAME
  --disable-protocols      disable all protocols
  --enable-indev=NAME      enable input device NAME
  --disable-indev=NAME     disable input device NAME
  --disable-indevs         disable input devices
  --enable-outdev=NAME     enable output device NAME
  --disable-outdev=NAME    disable output device NAME
  --disable-outdevs        disable output devices
  --disable-devices        disable all devices
  --enable-filter=NAME     enable filter NAME
  --disable-filter=NAME    disable filter NAME
  --disable-filters        disable all filters

External library support:

  Using any of the following switches will allow FFmpeg to link to the
  corresponding external library. All the components depending on that library
  will become enabled, if all their other dependencies are met and they are not
  explicitly disabled. E.g. --enable-libopus will enable linking to
  libopus and allow the libopus encoder to be built, unless it is
  specifically disabled with --disable-encoder=libopus.

  Note that only the system libraries are auto-detected. All the other external
  libraries must be explicitly enabled.

  Also note that the following help text describes the purpose of the libraries
  themselves, not all their features will necessarily be usable by FFmpeg.

  --disable-alsa           disable ALSA support [autodetect]
  --disable-appkit         disable Apple AppKit framework [autodetect]
  --disable-avfoundation   disable Apple AVFoundation framework [autodetect]
  --enable-avisynth        enable reading of AviSynth script files [no]
  --disable-bzlib          disable bzlib [autodetect]
  --disable-coreimage      disable Apple CoreImage framework [autodetect]
  --enable-chromaprint     enable audio fingerprinting with chromaprint [no]
  --enable-frei0r          enable frei0r video filtering [no]
  --enable-gcrypt          enable gcrypt, needed for rtmp(t)e support
                           if openssl, librtmp or gmp is not used [no]
  --enable-gmp             enable gmp, needed for rtmp(t)e support
                           if openssl or librtmp is not used [no]
  --enable-gnutls          enable gnutls, needed for https support
                           if openssl, libtls or mbedtls is not used [no]
  --disable-iconv          disable iconv [autodetect]
  --enable-jni             enable JNI support [no]
  --enable-ladspa          enable LADSPA audio filtering [no]
  --enable-lcms2           enable ICC profile support via LittleCMS 2 [no]
  --enable-libaom          enable AV1 video encoding/decoding via libaom [no]
  --enable-libaribb24      enable ARIB text and caption decoding via libaribb24 [no]
  --enable-libaribcaption  enable ARIB text and caption decoding via libaribcaption [no]
  --enable-libass          enable libass subtitles rendering,
                           needed for subtitles and ass filter [no]
  --enable-libbluray       enable BluRay reading using libbluray [no]
  --enable-libbs2b         enable bs2b DSP library [no]
  --enable-libcaca         enable textual display using libcaca [no]
  --enable-libcelt         enable CELT decoding via libcelt [no]
  --enable-libcdio         enable audio CD grabbing with libcdio [no]
  --enable-libcodec2       enable codec2 en/decoding using libcodec2 [no]
  --enable-libdav1d        enable AV1 decoding via libdav1d [no]
  --enable-libdavs2        enable AVS2 decoding via libdavs2 [no]
  --enable-libdc1394       enable IIDC-1394 grabbing using libdc1394
                           and libraw1394 [no]
  --enable-libfdk-aac      enable AAC de/encoding via libfdk-aac [no]
  --enable-libflite        enable flite (voice synthesis) support via libflite [no]
  --enable-libfontconfig   enable libfontconfig, useful for drawtext filter [no]
  --enable-libfreetype     enable libfreetype, needed for drawtext filter [no]
  --enable-libfribidi      enable libfribidi, improves drawtext filter [no]
  --enable-libglslang      enable GLSL->SPIRV compilation via libglslang [no]
  --enable-libgme          enable Game Music Emu via libgme [no]
  --enable-libgsm          enable GSM de/encoding via libgsm [no]
  --enable-libiec61883     enable iec61883 via libiec61883 [no]
  --enable-libilbc         enable iLBC de/encoding via libilbc [no]
  --enable-libjack         enable JACK audio sound server [no]
  --enable-libjxl          enable JPEG XL de/encoding via libjxl [no]
  --enable-libklvanc       enable Kernel Labs VANC processing [no]
  --enable-libkvazaar      enable HEVC encoding via libkvazaar [no]
  --enable-liblensfun      enable lensfun lens correction [no]
  --enable-libmodplug      enable ModPlug via libmodplug [no]
  --enable-libmp3lame      enable MP3 encoding via libmp3lame [no]
  --enable-libopencore-amrnb enable AMR-NB de/encoding via libopencore-amrnb [no]
  --enable-libopencore-amrwb enable AMR-WB decoding via libopencore-amrwb [no]
  --enable-libopencv       enable video filtering via libopencv [no]
  --enable-libopenh264     enable H.264 encoding via OpenH264 [no]
  --enable-libopenjpeg     enable JPEG 2000 de/encoding via OpenJPEG [no]
  --enable-libopenmpt      enable decoding tracked files via libopenmpt [no]
  --enable-libopenvino     enable OpenVINO as a DNN module backend
                           for DNN based filters like dnn_processing [no]
  --enable-libopus         enable Opus de/encoding via libopus [no]
  --enable-libplacebo      enable libplacebo library [no]
  --enable-libpulse        enable Pulseaudio input via libpulse [no]
  --enable-librabbitmq     enable RabbitMQ library [no]
  --enable-librav1e        enable AV1 encoding via rav1e [no]
  --enable-librist         enable RIST via librist [no]
  --enable-librsvg         enable SVG rasterization via librsvg [no]
  --enable-librubberband   enable rubberband needed for rubberband filter [no]
  --enable-librtmp         enable RTMP[E] support via librtmp [no]
  --enable-libshaderc      enable GLSL->SPIRV compilation via libshaderc [no]
  --enable-libshine        enable fixed-point MP3 encoding via libshine [no]
  --enable-libsmbclient    enable Samba protocol via libsmbclient [no]
  --enable-libsnappy       enable Snappy compression, needed for hap encoding [no]
  --enable-libsoxr         enable Include libsoxr resampling [no]
  --enable-libspeex        enable Speex de/encoding via libspeex [no]
  --enable-libsrt          enable Haivision SRT protocol via libsrt [no]
  --enable-libssh          enable SFTP protocol via libssh [no]
  --enable-libsvtav1       enable AV1 encoding via SVT [no]
  --enable-libtensorflow   enable TensorFlow as a DNN module backend
                           for DNN based filters like sr [no]
  --enable-libtesseract    enable Tesseract, needed for ocr filter [no]
  --enable-libtheora       enable Theora encoding via libtheora [no]
  --enable-libtls          enable LibreSSL (via libtls), needed for https support
                           if openssl, gnutls or mbedtls is not used [no]
  --enable-libtwolame      enable MP2 encoding via libtwolame [no]
  --enable-libuavs3d       enable AVS3 decoding via libuavs3d [no]
  --enable-libv4l2         enable libv4l2/v4l-utils [no]
  --enable-libvidstab      enable video stabilization using vid.stab [no]
  --enable-libvmaf         enable vmaf filter via libvmaf [no]
  --enable-libvo-amrwbenc  enable AMR-WB encoding via libvo-amrwbenc [no]
  --enable-libvorbis       enable Vorbis en/decoding via libvorbis,
                           native implementation exists [no]
  --enable-libvpx          enable VP8 and VP9 de/encoding via libvpx [no]
  --enable-libwebp         enable WebP encoding via libwebp [no]
  --enable-libx264         enable H.264 encoding via x264 [no]
  --enable-libx265         enable HEVC encoding via x265 [no]
  --enable-libxavs         enable AVS encoding via xavs [no]
  --enable-libxavs2        enable AVS2 encoding via xavs2 [no]
  --enable-libxcb          enable X11 grabbing using XCB [autodetect]
  --enable-libxcb-shm      enable X11 grabbing shm communication [autodetect]
  --enable-libxcb-xfixes   enable X11 grabbing mouse rendering [autodetect]
  --enable-libxcb-shape    enable X11 grabbing shape rendering [autodetect]
  --enable-libxvid         enable Xvid encoding via xvidcore,
                           native MPEG-4/Xvid encoder exists [no]
  --enable-libxml2         enable XML parsing using the C library libxml2, needed
                           for dash and imf demuxing support [no]
  --enable-libzimg         enable z.lib, needed for zscale filter [no]
  --enable-libzmq          enable message passing via libzmq [no]
  --enable-libzvbi         enable teletext support via libzvbi [no]
  --enable-lv2             enable LV2 audio filtering [no]
  --disable-lzma           disable lzma [autodetect]
  --enable-decklink        enable Blackmagic DeckLink I/O support [no]
  --enable-mbedtls         enable mbedTLS, needed for https support
                           if openssl, gnutls or libtls is not used [no]
  --enable-mediacodec      enable Android MediaCodec support [no]
  --enable-mediafoundation enable encoding via MediaFoundation [auto]
  --disable-metal          disable Apple Metal framework [autodetect]
  --enable-libmysofa       enable libmysofa, needed for sofalizer filter [no]
  --enable-openal          enable OpenAL 1.1 capture support [no]
  --enable-opencl          enable OpenCL processing [no]
  --enable-opengl          enable OpenGL rendering [no]
  --enable-openssl         enable openssl, needed for https support
                           if gnutls, libtls or mbedtls is not used [no]
  --enable-pocketsphinx    enable PocketSphinx, needed for asr filter [no]
  --disable-sndio          disable sndio support [autodetect]
  --disable-schannel       disable SChannel SSP, needed for TLS support on
                           Windows if openssl and gnutls are not used [autodetect]
  --disable-sdl2           disable sdl2 [autodetect]
  --disable-securetransport disable Secure Transport, needed for TLS support
                           on OSX if openssl and gnutls are not used [autodetect]
  --enable-vapoursynth     enable VapourSynth demuxer [no]
  --disable-xlib           disable xlib [autodetect]
  --disable-zlib           disable zlib [autodetect]

  The following libraries provide various hardware acceleration features:
  --disable-amf            disable AMF video encoding code [autodetect]
  --disable-audiotoolbox   disable Apple AudioToolbox code [autodetect]
  --enable-cuda-nvcc       enable Nvidia CUDA compiler [no]
  --disable-cuda-llvm      disable CUDA compilation using clang [autodetect]
  --disable-cuvid          disable Nvidia CUVID support [autodetect]
  --disable-d3d11va        disable Microsoft Direct3D 11 video acceleration code [autodetect]
  --disable-dxva2          disable Microsoft DirectX 9 video acceleration code [autodetect]
  --disable-ffnvcodec      disable dynamically linked Nvidia code [autodetect]
  --enable-libdrm          enable DRM code (Linux) [no]
  --enable-libmfx          enable Intel MediaSDK (AKA Quick Sync Video) code via libmfx [no]
  --enable-libvpl          enable Intel oneVPL code via libvpl if libmfx is not used [no]
  --enable-libnpp          enable Nvidia Performance Primitives-based code [no]
  --enable-mmal            enable Broadcom Multi-Media Abstraction Layer (Raspberry Pi) via MMAL [no]
  --disable-nvdec          disable Nvidia video decoding acceleration (via hwaccel) [autodetect]
  --disable-nvenc          disable Nvidia video encoding code [autodetect]
  --enable-omx             enable OpenMAX IL code [no]
  --enable-omx-rpi         enable OpenMAX IL code for Raspberry Pi [no]
  --enable-rkmpp           enable Rockchip Media Process Platform code [no]
  --disable-v4l2-m2m       disable V4L2 mem2mem code [autodetect]
  --disable-vaapi          disable Video Acceleration API (mainly Unix/Intel) code [autodetect]
  --disable-vdpau          disable Nvidia Video Decode and Presentation API for Unix code [autodetect]
  --disable-videotoolbox   disable VideoToolbox code [autodetect]
  --disable-vulkan         disable Vulkan code [autodetect]

Toolchain options:
  --arch=ARCH              select architecture []
  --cpu=CPU                select the minimum required CPU (affects
                           instruction selection, may crash on older CPUs)
  --cross-prefix=PREFIX    use PREFIX for compilation tools []
  --progs-suffix=SUFFIX    program name suffix []
  --enable-cross-compile   assume a cross-compiler is used
  --sysroot=PATH           root of cross-build tree
  --sysinclude=PATH        location of cross-build system headers
  --target-os=OS           compiler targets OS []
  --target-exec=CMD        command to run executables on target
  --target-path=DIR        path to view of build directory on target
  --target-samples=DIR     path to samples directory on target
  --tempprefix=PATH        force fixed dir/prefix instead of mktemp for checks
  --toolchain=NAME         set tool defaults according to NAME
                           (gcc-asan, clang-asan, gcc-msan, clang-msan,
                           gcc-tsan, clang-tsan, gcc-usan, clang-usan,
                           valgrind-massif, valgrind-memcheck,
                           msvc, icl, gcov, llvm-cov, hardened)
  --nm=NM                  use nm tool NM [nm -g]
  --ar=AR                  use archive tool AR [ar]
  --as=AS                  use assembler AS []
  --ln_s=LN_S              use symbolic link tool LN_S [ln -s -f]
  --strip=STRIP            use strip tool STRIP [strip]
  --windres=WINDRES        use windows resource compiler WINDRES [windres]
  --x86asmexe=EXE          use nasm-compatible assembler EXE [nasm]
  --cc=CC                  use C compiler CC [gcc]
  --cxx=CXX                use C compiler CXX [g++]
  --objcc=OCC              use ObjC compiler OCC [gcc]
  --dep-cc=DEPCC           use dependency generator DEPCC [gcc]
  --nvcc=NVCC              use Nvidia CUDA compiler NVCC or clang []
  --ld=LD                  use linker LD []
  --metalcc=METALCC        use metal compiler METALCC [xcrun -sdk macosx metal]
  --metallib=METALLIB      use metal linker METALLIB [xcrun -sdk macosx metallib]
  --pkg-config=PKGCONFIG   use pkg-config tool PKGCONFIG [pkg-config]
  --pkg-config-flags=FLAGS pass additional flags to pkgconf []
  --ranlib=RANLIB          use ranlib RANLIB [ranlib]
  --doxygen=DOXYGEN        use DOXYGEN to generate API doc [doxygen]
  --host-cc=HOSTCC         use host C compiler HOSTCC
  --host-cflags=HCFLAGS    use HCFLAGS when compiling for host
  --host-cppflags=HCPPFLAGS use HCPPFLAGS when compiling for host
  --host-ld=HOSTLD         use host linker HOSTLD
  --host-ldflags=HLDFLAGS  use HLDFLAGS when linking for host
  --host-extralibs=HLIBS   use libs HLIBS when linking for host
  --host-os=OS             compiler host OS []
  --extra-cflags=ECFLAGS   add ECFLAGS to CFLAGS []
  --extra-cxxflags=ECFLAGS add ECFLAGS to CXXFLAGS []
  --extra-objcflags=FLAGS  add FLAGS to OBJCFLAGS []
  --extra-ldflags=ELDFLAGS add ELDFLAGS to LDFLAGS []
  --extra-ldexeflags=ELDFLAGS add ELDFLAGS to LDEXEFLAGS []
  --extra-ldsoflags=ELDFLAGS add ELDFLAGS to LDSOFLAGS []
  --extra-libs=ELIBS       add ELIBS []
  --extra-version=STRING   version string suffix []
  --optflags=OPTFLAGS      override optimization-related compiler flags
  --nvccflags=NVCCFLAGS    override nvcc flags []
  --build-suffix=SUFFIX    library name suffix []
  --enable-pic             build position-independent code
  --enable-thumb           compile for Thumb instruction set
  --enable-lto[=arg]       use link-time optimization
  --env="ENV=override"     override the environment variables

Advanced options (experts only):
  --malloc-prefix=PREFIX   prefix malloc and related names with PREFIX
  --custom-allocator=NAME  use a supported custom allocator
  --disable-symver         disable symbol versioning
  --enable-hardcoded-tables use hardcoded tables instead of runtime generation
  --disable-safe-bitstream-reader
                           disable buffer boundary checking in bitreaders
                           (faster, but may crash)
  --sws-max-filter-size=N  the max filter size swscale uses [256]

Optimization options (experts only):
  --disable-asm            disable all assembly optimizations
  --disable-altivec        disable AltiVec optimizations
  --disable-vsx            disable VSX optimizations
  --disable-power8         disable POWER8 optimizations
  --disable-amd3dnow       disable 3DNow! optimizations
  --disable-amd3dnowext    disable 3DNow! extended optimizations
  --disable-mmx            disable MMX optimizations
  --disable-mmxext         disable MMXEXT optimizations
  --disable-sse            disable SSE optimizations
  --disable-sse2           disable SSE2 optimizations
  --disable-sse3           disable SSE3 optimizations
  --disable-ssse3          disable SSSE3 optimizations
  --disable-sse4           disable SSE4 optimizations
  --disable-sse42          disable SSE4.2 optimizations
  --disable-avx            disable AVX optimizations
  --disable-xop            disable XOP optimizations
  --disable-fma3           disable FMA3 optimizations
  --disable-fma4           disable FMA4 optimizations
  --disable-avx2           disable AVX2 optimizations
  --disable-avx512         disable AVX-512 optimizations
  --disable-avx512icl      disable AVX-512ICL optimizations
  --disable-aesni          disable AESNI optimizations
  --disable-armv5te        disable armv5te optimizations
  --disable-armv6          disable armv6 optimizations
  --disable-armv6t2        disable armv6t2 optimizations
  --disable-vfp            disable VFP optimizations
  --disable-neon           disable NEON optimizations
  --disable-dotprod        disable DOTPROD optimizations
  --disable-i8mm           disable I8MM optimizations
  --disable-inline-asm     disable use of inline assembly
  --disable-x86asm         disable use of standalone x86 assembly
  --disable-mipsdsp        disable MIPS DSP ASE R1 optimizations
  --disable-mipsdspr2      disable MIPS DSP ASE R2 optimizations
  --disable-msa            disable MSA optimizations
  --disable-mipsfpu        disable floating point MIPS optimizations
  --disable-mmi            disable Loongson MMI optimizations
  --disable-lsx            disable Loongson LSX optimizations
  --disable-lasx           disable Loongson LASX optimizations
  --disable-rvv            disable RISC-V Vector optimizations
  --disable-fast-unaligned consider unaligned accesses slow

Developer options (useful when working on FFmpeg itself):
  --disable-debug          disable debugging symbols
  --enable-debug=LEVEL     set the debug level []
  --disable-optimizations  disable compiler optimizations
  --enable-extra-warnings  enable more compiler warnings
  --disable-stripping      disable stripping of executables and shared libraries
  --assert-level=level     0(default), 1 or 2, amount of assertion testing,
                           2 causes a slowdown at runtime.
  --enable-memory-poisoning fill heap uninitialized allocated space with arbitrary data
  --valgrind=VALGRIND      run "make fate" tests through valgrind to detect memory
                           leaks and errors, using the specified valgrind binary.
                           Cannot be combined with --target-exec
  --enable-ftrapv          Trap arithmetic overflows
  --samples=PATH           location of test samples for FATE, if not set use
                           $FATE_SAMPLES at make invocation time.
  --enable-neon-clobber-test check NEON registers for clobbering (should be
                           used only for debugging purposes)
  --enable-xmm-clobber-test check XMM registers for clobbering (Win64-only;
                           should be used only for debugging purposes)
  --enable-random          randomly enable/disable components
  --disable-random
  --enable-random=LIST     randomly enable/disable specific components or
  --disable-random=LIST    component groups. LIST is a comma-separated list
                           of NAME[:PROB] entries where NAME is a component
                           (group) and PROB the probability associated with
                           NAME (default 0.5).
  --random-seed=VALUE      seed value for --enable/disable-random
  --disable-valgrind-backtrace do not print a backtrace under Valgrind
                           (only applies to --disable-optimizations builds)
  --enable-ossfuzz         Enable building fuzzer tool
  --libfuzzer=PATH         path to libfuzzer
  --ignore-tests=TESTS     comma-separated list (without "fate-" prefix
                           in the name) of tests whose result is ignored
  --enable-linux-perf      enable Linux Performance Monitor API
  --enable-macos-kperf     enable macOS kperf (private) API
  --disable-large-tests    disable tests that use a large amount of memory
  --disable-ptx-compression don't compress CUDA PTX code even when possible

NOTE: Object files are built at the place where configure is launched.

```

5. FFmpeg 6.0 Configuration

```bash
install prefix            ../../ffmpeg_shared
source path               .
C compiler                cl.exe
C library                 msvcrt
ARCH                      x86 (generic)
big-endian                no
runtime cpu detection     yes
standalone assembly       yes
x86 assembler             nasm
MMX enabled               yes
MMXEXT enabled            yes
3DNow! enabled            yes
3DNow! extended enabled   yes
SSE enabled               yes
SSSE3 enabled             yes
AESNI enabled             yes
AVX enabled               yes
AVX2 enabled              yes
AVX-512 enabled           yes
AVX-512ICL enabled        yes
XOP enabled               yes
FMA3 enabled              yes
FMA4 enabled              yes
i686 features enabled     yes
CMOV is fast              yes
EBX available             no
EBP available             no
debug symbols             yes
strip symbols             no
optimize for size         no
optimizations             yes
static                    no
shared                    yes
postprocessing support    yes
network support           yes
threading support         w32threads
safe bitstream reader     yes
texi2html enabled         no
perl enabled              yes
pod2man enabled           yes
makeinfo enabled          no
makeinfo supports HTML    no
xmllint enabled           yes

External libraries:
libx264                 libx265                 mediafoundation         schannel

External libraries providing hardware acceleration:
cuda                    cuda_nvcc               d3d11va                 ffnvcodec               nvdec
cuda_llvm               cuvid                   dxva2                   libnpp                  nvenc

Libraries:
avcodec                 avfilter                avutil                  swresample
avdevice                avformat                postproc                swscale

Programs:
ffmpeg                  ffprobe

Enabled decoders:
aac                     atrac3                  ffv1                    mp1                     pcm_vidc                tiertexseqvideo
aac_fixed               atrac3al                ffvhuff                 mp1float                pcx                     tiff
aac_latm                atrac3p                 ffwavesynth             mp2                     pfm                     tmv
aasc                    atrac3pal               fic                     mp2float                pgm                     truehd
ac3                     atrac9                  fits                    mp3                     pgmyuv                  truemotion1
ac3_fixed               aura                    flac                    mp3adu                  pgssub                  truemotion2
acelp_kelvin            aura2                   flic                    mp3adufloat             pgx                     truemotion2rt
adpcm_4xm               av1                     flv                     mp3float                phm                     truespeech
adpcm_adx               av1_cuvid               fmvc                    mp3on4                  photocd                 tscc2
adpcm_afc               avrn                    fourxm                  mp3on4float             pictor                  tta
adpcm_agm               avrp                    fraps                   mpc7                    pixlet                  twinvq
adpcm_aica              avs                     frwu                    mpc8                    pjs                     txd
adpcm_argo              avui                    ftr                     mpeg1_cuvid             ppm                     ulti
adpcm_ct                ayuv                    g723_1                  mpeg1video              prores                  utvideo
adpcm_dtk               bethsoftvid             g729                    mpeg2_cuvid             prosumer                v210
adpcm_ea                bfi                     gdv                     mpeg2video              psd                     v210x
adpcm_ea_maxis_xa       bink                    gem                     mpeg4                   ptx                     v308
adpcm_ea_r1             binkaudio_dct           gif                     mpeg4_cuvid             qcelp                   v408
adpcm_ea_r2             binkaudio_rdft          gremlin_dpcm            mpegvideo               qdm2                    v410
adpcm_ea_r3             bintext                 gsm                     mpl2                    qdmc                    vb
adpcm_ea_xas            bitpacked               gsm_ms                  msa1                    qdraw                   vble
adpcm_g722              bmp                     h261                    msmpeg4v1               qoi                     vbn
adpcm_g726              bmv_audio               h263                    msmpeg4v2               qpeg                    vc1
adpcm_g726le            bmv_video               h263i                   msmpeg4v3               qtrle                   vc1_cuvid
adpcm_ima_acorn         bonk                    h263p                   msnsiren                r10k                    vc1image
adpcm_ima_alp           brender_pix             h264                    msp2                    r210                    vcr1
adpcm_ima_amv           c93                     h264_cuvid              msrle                   ra_144                  vmdaudio
adpcm_ima_apc           cavs                    hap                     mss1                    ra_288                  vmdvideo
adpcm_ima_apm           cbd2_dpcm               hca                     mss2                    ralf                    vmnc
adpcm_ima_cunning       ccaption                hcom                    msvideo1                rawvideo                vnull
adpcm_ima_dat4          cdgraphics              hdr                     mszh                    realtext                vorbis
adpcm_ima_dk3           cdtoons                 hevc                    mts2                    rka                     vp3
adpcm_ima_dk4           cdxl                    hevc_cuvid              mv30                    rl2                     vp4
adpcm_ima_ea_eacs       cfhd                    hnm4_video              mvc1                    roq                     vp5
adpcm_ima_ea_sead       cinepak                 hq_hqa                  mvc2                    roq_dpcm                vp6
adpcm_ima_iss           clearvideo              hqx                     mvdv                    rpza                    vp6a
adpcm_ima_moflex        cljr                    huffyuv                 mxpeg                   rtv1                    vp6f
adpcm_ima_mtf           cllc                    hymt                    nellymoser              rv10                    vp7
adpcm_ima_oki           comfortnoise            iac                     notchlc                 rv20                    vp8
adpcm_ima_qt            cook                    idcin                   nuv                     rv30                    vp8_cuvid
adpcm_ima_rad           cpia                    idf                     on2avc                  rv40                    vp9
adpcm_ima_smjpeg        cri                     iff_ilbm                opus                    s302m                   vp9_cuvid
adpcm_ima_ssi           cscd                    ilbc                    paf_audio               sami                    vplayer
adpcm_ima_wav           cyuv                    imc                     paf_video               sanm                    vqa
adpcm_ima_ws            dca                     imm4                    pam                     sbc                     vqc
adpcm_ms                dds                     imm5                    pbm                     scpr                    wady_dpcm
adpcm_mtaf              derf_dpcm               indeo2                  pcm_alaw                sdx2_dpcm               wavarc
adpcm_psx               dfa                     indeo3                  pcm_bluray              sga                     wavpack
adpcm_sbpro_2           dfpwm                   indeo4                  pcm_dvd                 sgi                     wbmp
adpcm_sbpro_3           dirac                   indeo5                  pcm_f16le               sgirle                  webp
adpcm_sbpro_4           dnxhd                   interplay_acm           pcm_f24le               sheervideo              webvtt
adpcm_swf               dolby_e                 interplay_dpcm          pcm_f32be               shorten                 wmalossless
adpcm_thp               dpx                     interplay_video         pcm_f32le               simbiosis_imx           wmapro
adpcm_thp_le            dsd_lsbf                ipu                     pcm_f64be               sipr                    wmav1
adpcm_vima              dsd_lsbf_planar         jacosub                 pcm_f64le               siren                   wmav2
adpcm_xa                dsd_msbf                jpeg2000                pcm_lxf                 smackaud                wmavoice
adpcm_xmd               dsd_msbf_planar         jpegls                  pcm_mulaw               smacker                 wmv1
adpcm_yamaha            dsicinaudio             jv                      pcm_s16be               smc                     wmv2
adpcm_zork              dsicinvideo             kgv1                    pcm_s16be_planar        smvjpeg                 wmv3
agm                     dss_sp                  kmvc                    pcm_s16le               snow                    wmv3image
aic                     dst                     lagarith                pcm_s16le_planar        sol_dpcm                wnv1
alac                    dvaudio                 loco                    pcm_s24be               sonic                   wrapped_avframe
alias_pix               dvbsub                  m101                    pcm_s24daud             sp5x                    ws_snd1
als                     dvdsub                  mace3                   pcm_s24le               speedhq                 xan_dpcm
amrnb                   dvvideo                 mace6                   pcm_s24le_planar        speex                   xan_wc3
amrwb                   dxtory                  magicyuv                pcm_s32be               srt                     xan_wc4
amv                     dxv                     mdec                    pcm_s32le               ssa                     xbin
anm                     eac3                    media100                pcm_s32le_planar        stl                     xbm
ansi                    eacmv                   metasound               pcm_s64be               subrip                  xface
anull                   eamad                   microdvd                pcm_s64le               subviewer               xl
apac                    eatgq                   mimic                   pcm_s8                  subviewer1              xma1
ape                     eatgv                   misc4                   pcm_s8_planar           sunrast                 xma2
aptx                    eatqi                   mjpeg                   pcm_sga                 svq1                    xpm
aptx_hd                 eightbps                mjpeg_cuvid             pcm_u16be               svq3                    xsub
arbc                    eightsvx_exp            mjpegb                  pcm_u16le               tak                     xwd
argo                    eightsvx_fib            mlp                     pcm_u24be               targa                   y41p
ass                     escape124               mmvideo                 pcm_u24le               targa_y216              ylc
asv1                    escape130               mobiclip                pcm_u32be               text                    yop
asv2                    evrc                    motionpixels            pcm_u32le               theora                  yuv4
atrac1                  fastaudio               movtext                 pcm_u8                  thp                     zero12v

Enabled encoders:
a64multi                av1_nvenc               hevc_mf                 pcm_f64be               ppm                     truehd
a64multi5               avrp                    hevc_nvenc              pcm_f64le               prores                  tta
aac                     avui                    huffyuv                 pcm_mulaw               prores_aw               ttml
aac_mf                  ayuv                    jpeg2000                pcm_s16be               prores_ks               utvideo
ac3                     bitpacked               jpegls                  pcm_s16be_planar        qoi                     v210
ac3_fixed               bmp                     libx264                 pcm_s16le               qtrle                   v308
ac3_mf                  cfhd                    libx264rgb              pcm_s16le_planar        r10k                    v408
adpcm_adx               cinepak                 libx265                 pcm_s24be               r210                    v410
adpcm_argo              cljr                    ljpeg                   pcm_s24daud             ra_144                  vbn
adpcm_g722              comfortnoise            magicyuv                pcm_s24le               rawvideo                vc2
adpcm_g726              dca                     mjpeg                   pcm_s24le_planar        roq                     vnull
adpcm_g726le            dfpwm                   mlp                     pcm_s32be               roq_dpcm                vorbis
adpcm_ima_alp           dnxhd                   movtext                 pcm_s32le               rpza                    wavpack
adpcm_ima_amv           dpx                     mp2                     pcm_s32le_planar        rv10                    wbmp
adpcm_ima_apm           dvbsub                  mp2fixed                pcm_s64be               rv20                    webvtt
adpcm_ima_qt            dvdsub                  mp3_mf                  pcm_s64le               s302m                   wmav1
adpcm_ima_ssi           dvvideo                 mpeg1video              pcm_s8                  sbc                     wmav2
adpcm_ima_wav           eac3                    mpeg2video              pcm_s8_planar           sgi                     wmv1
adpcm_ima_ws            ffv1                    mpeg4                   pcm_u16be               smc                     wmv2
adpcm_ms                ffvhuff                 msmpeg4v2               pcm_u16le               snow                    wrapped_avframe
adpcm_swf               fits                    msmpeg4v3               pcm_u24be               sonic                   xbm
adpcm_yamaha            flac                    msvideo1                pcm_u24le               sonic_ls                xface
alac                    flv                     nellymoser              pcm_u32be               speedhq                 xsub
alias_pix               g723_1                  opus                    pcm_u32le               srt                     xwd
amv                     gif                     pam                     pcm_u8                  ssa                     y41p
anull                   h261                    pbm                     pcm_vidc                subrip                  yuv4
aptx                    h263                    pcm_alaw                pcx                     sunrast
aptx_hd                 h263p                   pcm_bluray              pfm                     svq1
ass                     h264_mf                 pcm_dvd                 pgm                     targa
asv1                    h264_nvenc              pcm_f32be               pgmyuv                  text
asv2                    hdr                     pcm_f32le               phm                     tiff

Enabled hwaccels:
av1_d3d11va             h264_dxva2              mjpeg_nvdec             mpeg4_nvdec             vp9_d3d11va             wmv3_dxva2
av1_d3d11va2            h264_nvdec              mpeg1_nvdec             vc1_d3d11va             vp9_d3d11va2            wmv3_nvdec
av1_dxva2               hevc_d3d11va            mpeg2_d3d11va           vc1_d3d11va2            vp9_dxva2
av1_nvdec               hevc_d3d11va2           mpeg2_d3d11va2          vc1_dxva2               vp9_nvdec
h264_d3d11va            hevc_dxva2              mpeg2_dxva2             vc1_nvdec               wmv3_d3d11va
h264_d3d11va2           hevc_nvdec              mpeg2_nvdec             vp8_nvdec               wmv3_d3d11va2

Enabled parsers:
aac                     cook                    dvdsub                  hdr                     opus                    vorbis
aac_latm                cri                     flac                    hevc                    png                     vp3
ac3                     dca                     ftr                     ipu                     pnm                     vp8
adx                     dirac                   g723_1                  jpeg2000                qoi                     vp9
amr                     dnxhd                   g729                    misc4                   rv30                    webp
av1                     dolby_e                 gif                     mjpeg                   rv40                    xbm
avs2                    dpx                     gsm                     mlp                     sbc                     xma
avs3                    dvaudio                 h261                    mpeg4video              sipr                    xwd
bmp                     dvbsub                  h263                    mpegaudio               tak
cavsvideo               dvd_nav                 h264                    mpegvideo               vc1

Enabled demuxers:
aa                      cdxl                    iff                     lmlm4                   pcm_s16le               spdif
aac                     cine                    ifv                     loas                    pcm_s24be               srt
aax                     codec2                  ilbc                    lrc                     pcm_s24le               stl
ac3                     codec2raw               image2                  luodat                  pcm_s32be               str
ace                     concat                  image2_alias_pix        lvf                     pcm_s32le               subviewer
acm                     data                    image2_brender_pix      lxf                     pcm_s8                  subviewer1
act                     daud                    image2pipe              m4v                     pcm_u16be               sup
adf                     dcstr                   image_bmp_pipe          matroska                pcm_u16le               svag
adp                     derf                    image_cri_pipe          mca                     pcm_u24be               svs
ads                     dfa                     image_dds_pipe          mcc                     pcm_u24le               swf
adx                     dfpwm                   image_dpx_pipe          mgsts                   pcm_u32be               tak
aea                     dhav                    image_exr_pipe          microdvd                pcm_u32le               tedcaptions
afc                     dirac                   image_gem_pipe          mjpeg                   pcm_u8                  thp
aiff                    dnxhd                   image_gif_pipe          mjpeg_2000              pcm_vidc                threedostr
aix                     dsf                     image_hdr_pipe          mlp                     pdv                     tiertexseq
alp                     dsicin                  image_j2k_pipe          mlv                     pjs                     tmv
amr                     dss                     image_jpeg_pipe         mm                      pmp                     truehd
amrnb                   dts                     image_jpegls_pipe       mmf                     pp_bnk                  tta
amrwb                   dtshd                   image_jpegxl_pipe       mods                    pva                     tty
anm                     dv                      image_pam_pipe          moflex                  pvf                     txd
apac                    dvbsub                  image_pbm_pipe          mov                     qcp                     ty
apc                     dvbtxt                  image_pcx_pipe          mp3                     r3d                     v210
ape                     dxa                     image_pfm_pipe          mpc                     rawvideo                v210x
apm                     ea                      image_pgm_pipe          mpc8                    realtext                vag
apng                    ea_cdata                image_pgmyuv_pipe       mpegps                  redspark                vc1
aptx                    eac3                    image_pgx_pipe          mpegts                  rka                     vc1t
aptx_hd                 epaf                    image_phm_pipe          mpegtsraw               rl2                     vividas
aqtitle                 ffmetadata              image_photocd_pipe      mpegvideo               rm                      vivo
argo_asf                filmstrip               image_pictor_pipe       mpjpeg                  roq                     vmd
argo_brp                fits                    image_png_pipe          mpl2                    rpl                     vobsub
argo_cvg                flac                    image_ppm_pipe          mpsub                   rsd                     voc
asf                     flic                    image_psd_pipe          msf                     rso                     vpk
asf_o                   flv                     image_qdraw_pipe        msnwc_tcp               rtp                     vplayer
ass                     fourxm                  image_qoi_pipe          msp                     rtsp                    vqf
ast                     frm                     image_sgi_pipe          mtaf                    s337m                   w64
au                      fsb                     image_sunrast_pipe      mtv                     sami                    wady
av1                     fwse                    image_svg_pipe          musx                    sap                     wav
avi                     g722                    image_tiff_pipe         mv                      sbc                     wavarc
avr                     g723_1                  image_vbn_pipe          mvi                     sbg                     wc3
avs                     g726                    image_webp_pipe         mxf                     scc                     webm_dash_manifest
avs2                    g726le                  image_xbm_pipe          mxg                     scd                     webvtt
avs3                    g729                    image_xpm_pipe          nc                      sdns                    wsaud
bethsoftvid             gdv                     image_xwd_pipe          nistsphere              sdp                     wsd
bfi                     genh                    ingenient               nsp                     sdr2                    wsvqa
bfstm                   gif                     ipmovie                 nsv                     sds                     wtv
bink                    gsm                     ipu                     nut                     sdx                     wv
binka                   gxf                     ircam                   nuv                     segafilm                wve
bintext                 h261                    iss                     obu                     ser                     xa
bit                     h263                    iv8                     ogg                     sga                     xbin
bitpacked               h264                    ivf                     oma                     shorten                 xmd
bmv                     hca                     ivr                     paf                     siff                    xmv
boa                     hcom                    jacosub                 pcm_alaw                simbiosis_imx           xvag
bonk                    hevc                    jpegxl_anim             pcm_f32be               sln                     xwma
brstm                   hls                     jv                      pcm_f32le               smacker                 yop
c93                     hnm                     kux                     pcm_f64be               smjpeg                  yuv4mpegpipe
caf                     ico                     kvag                    pcm_f64le               smush
cavsvideo               idcin                   laf                     pcm_mulaw               sol
cdg                     idf                     live_flv                pcm_s16be               sox

Enabled muxers:
a64                     crc                     h263                    mp3                     pcm_s16le               sox
ac3                     dash                    h264                    mp4                     pcm_s24be               spdif
adts                    data                    hash                    mpeg1system             pcm_s24le               spx
adx                     daud                    hds                     mpeg1vcd                pcm_s32be               srt
aiff                    dfpwm                   hevc                    mpeg1video              pcm_s32le               stream_segment
alp                     dirac                   hls                     mpeg2dvd                pcm_s8                  streamhash
amr                     dnxhd                   ico                     mpeg2svcd               pcm_u16be               sup
amv                     dts                     ilbc                    mpeg2video              pcm_u16le               swf
apm                     dv                      image2                  mpeg2vob                pcm_u24be               tee
apng                    eac3                    image2pipe              mpegts                  pcm_u24le               tg2
aptx                    f4v                     ipod                    mpjpeg                  pcm_u32be               tgp
aptx_hd                 ffmetadata              ircam                   mxf                     pcm_u32le               truehd
argo_asf                fifo                    ismv                    mxf_d10                 pcm_u8                  tta
argo_cvg                fifo_test               ivf                     mxf_opatom              pcm_vidc                ttml
asf                     filmstrip               jacosub                 null                    psp                     uncodedframecrc
asf_stream              fits                    kvag                    nut                     rawvideo                vc1
ass                     flac                    latm                    obu                     rm                      vc1t
ast                     flv                     lrc                     oga                     roq                     voc
au                      framecrc                m4v                     ogg                     rso                     w64
avi                     framehash               matroska                ogv                     rtp                     wav
avif                    framemd5                matroska_audio          oma                     rtp_mpegts              webm
avm2                    g722                    md5                     opus                    rtsp                    webm_chunk
avs2                    g723_1                  microdvd                pcm_alaw                sap                     webm_dash_manifest
avs3                    g726                    mjpeg                   pcm_f32be               sbc                     webp
bit                     g726le                  mkvtimestamp_v2         pcm_f32le               scc                     webvtt
caf                     gif                     mlp                     pcm_f64be               segafilm                wsaud
cavsvideo               gsm                     mmf                     pcm_f64le               segment                 wtv
codec2                  gxf                     mov                     pcm_mulaw               smjpeg                  wv
codec2raw               h261                    mp2                     pcm_s16be               smoothstreaming         yuv4mpegpipe

Enabled protocols:
async                   fd                      hls                     ipns_gateway            rtmp                    subfile
cache                   ffrtmphttp              http                    md5                     rtmps                   tcp
concat                  file                    httpproxy               mmsh                    rtmpt                   tee
concatf                 ftp                     https                   mmst                    rtmpts                  tls
crypto                  gopher                  icecast                 pipe                    rtp                     udp
data                    gophers                 ipfs_gateway            prompeg                 srtp                    udplite

Enabled filters:
a3dscope                asetnsamples            crossfeed               hdcd                    owdenoise               sinc
abench                  asetpts                 crystalizer             headphone               pad                     sine
abitscope               asetrate                cue                     hflip                   pal100bars              siti
acompressor             asettb                  curves                  highpass                pal75bars               smartblur
acontrast               ashowinfo               datascope               highshelf               palettegen              smptebars
acopy                   asidedata               dblur                   hilbert                 paletteuse              smptehdbars
acrossfade              asoftclip               dcshift                 histeq                  pan                     sobel
acrossover              aspectralstats          dctdnoiz                histogram               perms                   spectrumsynth
acrusher                asplit                  ddagrab                 hqdn3d                  perspective             speechnorm
acue                    astats                  deband                  hqx                     phase                   split
addroi                  astreamselect           deblock                 hstack                  photosensitivity        spp
adeclick                asubboost               decimate                hsvhold                 pixdesctest             sr
adeclip                 asubcut                 deconvolve              hsvkey                  pixelize                ssim
adecorrelate            asupercut               dedot                   hue                     pixscope                ssim360
adelay                  asuperpass              deesser                 huesaturation           pp                      stereo3d
adenorm                 asuperstop              deflate                 hwdownload              pp7                     stereotools
aderivative             atadenoise              deflicker               hwmap                   premultiply             stereowiden
adrawgraph              atempo                  dejudder                hwupload                prewitt                 streamselect
adrc                    atilt                   delogo                  hwupload_cuda           pseudocolor             super2xsai
adynamicequalizer       atrim                   derain                  hysteresis              psnr                    superequalizer
adynamicsmooth          avectorscope            deshake                 identity                pullup                  surround
aecho                   avgblur                 despill                 idet                    qp                      swaprect
aemphasis               avsynctest              detelecine              il                      random                  swapuv
aeval                   axcorrelate             dialoguenhance          inflate                 readeia608              tblend
aevalsrc                backgroundkey           dilation                interlace               readvitc                telecine
aexciter                bandpass                displace                interleave              realtime                testsrc
afade                   bandreject              dnn_classify            join                    remap                   testsrc2
afdelaysrc              bass                    dnn_detect              kerndeint               removegrain             thistogram
afftdn                  bbox                    dnn_processing          kirsch                  removelogo              threshold
afftfilt                bench                   doubleweave             lagfun                  repeatfields            thumbnail
afifo                   bilateral               drawbox                 latency                 replaygain              thumbnail_cuda
afir                    bilateral_cuda          drawgraph               lenscorrection          reverse                 tile
afireqsrc               biquad                  drawgrid                life                    rgbashift               tiltshelf
afirsrc                 bitplanenoise           drmeter                 limitdiff               rgbtestsrc              tinterlace
aformat                 blackdetect             dynaudnorm              limiter                 roberts                 tlut2
afreqshift              blackframe              earwax                  loop                    rotate                  tmedian
afwtdn                  blend                   ebur128                 loudnorm                sab                     tmidequalizer
agate                   blockdetect             edgedetect              lowpass                 scale                   tmix
agraphmonitor           blurdetect              elbg                    lowshelf                scale2ref               tonemap
ahistogram              bm3d                    entropy                 lumakey                 scale2ref_npp           tpad
aiir                    boxblur                 epx                     lut                     scale_cuda              transpose
aintegral               bwdif                   eq                      lut1d                   scale_npp               transpose_npp
ainterleave             cas                     equalizer               lut2                    scdet                   treble
alatency                ccrepack                erosion                 lut3d                   scharr                  tremolo
alimiter                cellauto                estdif                  lutrgb                  scroll                  trim
allpass                 channelmap              exposure                lutyuv                  segment                 unpremultiply
allrgb                  channelsplit            extractplanes           mandelbrot              select                  unsharp
allyuv                  chorus                  extrastereo             maskedclamp             selectivecolor          untile
aloop                   chromahold              fade                    maskedmax               sendcmd                 uspp
alphaextract            chromakey               feedback                maskedmerge             separatefields          v360
alphamerge              chromakey_cuda          fftdnoiz                maskedmin               setdar                  vaguedenoiser
amerge                  chromanr                fftfilt                 maskedthreshold         setfield                varblur
ametadata               chromashift             field                   maskfun                 setparams               vectorscope
amix                    ciescope                fieldhint               mcdeint                 setpts                  vflip
amovie                  codecview               fieldmatch              mcompand                setrange                vfrdet
amplify                 color                   fieldorder              median                  setsar                  vibrance
amultiply               colorbalance            fifo                    mergeplanes             settb                   vibrato
anequalizer             colorchannelmixer       fillborders             mestimate               sharpen_npp             vif
anlmdn                  colorchart              find_rect               metadata                shear                   vignette
anlmf                   colorcontrast           firequalizer            midequalizer            showcqt                 virtualbass
anlms                   colorcorrect            flanger                 minterpolate            showcwt                 vmafmotion
anoisesrc               colorhold               floodfill               mix                     showfreqs               volume
anull                   colorize                format                  monochrome              showinfo                volumedetect
anullsink               colorkey                fps                     morpho                  showpalette             vstack
anullsrc                colorlevels             framepack               movie                   showspatial             w3fdif
apad                    colormap                framerate               mpdecimate              showspectrum            waveform
aperms                  colormatrix             framestep               mptestsrc               showspectrumpic         weave
aphasemeter             colorspace              freezedetect            msad                    showvolume              xbr
aphaser                 colorspace_cuda         freezeframes            multiply                showwaves               xcorrelate
aphaseshift             colorspectrum           fspp                    negate                  showwavespic            xfade
apsyclip                colortemperature        gblur                   nlmeans                 shuffleframes           xmedian
apulsator               compand                 geq                     nnedi                   shufflepixels           xstack
arealtime               compensationdelay       gradfun                 noformat                shuffleplanes           yadif
aresample               concat                  gradients               noise                   sidechaincompress       yadif_cuda
areverse                convolution             graphmonitor            normalize               sidechaingate           yaepblur
arls                    convolve                grayworld               null                    sidedata                yuvtestsrc
arnndn                  copy                    greyedge                nullsink                sierpinski              zoneplate
asdr                    corr                    guided                  nullsrc                 signalstats             zoompan
asegment                cover_rect              haas                    oscilloscope            signature
aselect                 crop                    haldclut                overlay                 silencedetect
asendcmd                cropdetect              haldclutsrc             overlay_cuda            silenceremove

Enabled bsfs:
aac_adtstoasc           dump_extradata          h264_redundant_pps      mjpega_dump_header      opus_metadata           trace_headers
av1_frame_merge         dv_error_marker         hapqa_extract           mov2textsub             pcm_rechunk             truehd_core
av1_frame_split         eac3_core               hevc_metadata           mp3_header_decompress   pgs_frame_merge         vp9_metadata
av1_metadata            extract_extradata       hevc_mp4toannexb        mpeg2_metadata          prores_metadata         vp9_raw_reorder
chomp                   filter_units            imx_dump_header         mpeg4_unpack_bframes    remove_extradata        vp9_superframe
dca_core                h264_metadata           media100_to_mjpegb      noise                   setts                   vp9_superframe_split
dts2pts                 h264_mp4toannexb        mjpeg2jpeg              null                    text2movsub

Enabled indevs:
dshow                   gdigrab                 lavfi                   vfwcap

Enabled outdevs:

License: nonfree and unredistributable
```