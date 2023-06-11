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
CC=cl ./configure --prefix=/usr/local --enable-static
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

| Format                               | Supported | Type                                     | Option                                                                              | Repository                                                                      |
| ------------------------------------ | --------- | ---------------------------------------- | ----------------------------------------------------------------------------------- | ------------------------------------------------------------------------------- |
| H.264 / AVC                          | &#10004;  | Video                                    | --enable-libx264                                                                    | [libx264](https://code.videolan.org/videolan/x264.git)                          |
| H.265 / HEVC                         | &#10004;  | Video                                    | --enable-libx265                                                                    | [libx265](https://bitbucket.org/multicoreware/x265_git.git)                     |
| VP8 / VP9                            |           | Video                                    | --enable-libvpx                                                                     | [libvpx](https://chromium.googlesource.com/webm/libvpx)                         |
| AV1                                  |           | Video                                    | --enable-libaom                                                                     | [libaom](https://aomedia.googlesource.com/aom)                                  |
| MPEG-4 Part 2 / MPEG-4               |           | Video                                    | --enable-libxvid                                                                    | [libxvid](https://labs.xvid.com/source/)                                        |
| Theora video compression             |           | Video                                    | --enable-libtheora                                                                  | [libtheora](https://www.theora.org/downloads/)                                  |
| Video Multi-Method Assessment Fusion |           | Video / Filter                           | --enable-libvmaf                                                                    | [libvmaf](https://github.com/Netflix/vmaf.git)                                  |
| VidStab                              |           | Video                                    | --enable-libvidstab                                                                 | [libvidstab](https://github.com/georgmartius/vid.stab.git)                      |
| .mp4                                 | &#10004;  | Video / Muxer                            | --enable-muxer=mp4                                                                  | built-in                                                                        |
| .mkv                                 | &#10004;  | Video / Muxer                            | --enable-muxer=matroska                                                             | built-in                                                                        |
| .gif                                 | &#10004;  | Video / Muxer                            | --enable-muxer=gif                                                                  | built-in                                                                        |
|                                      |           |                                          |                                                                                     |                                                                                 |
| MP3                                  | &#10004;  | Audio                                    | --enable-libmp3lame                                                                 | [libmp3lame](https://lame.sourceforge.io/)                                      |
| AAC                                  | &#10004;  | Audio / Encoder / Decoder                | --enable-encoder=aac <br> --enable-decoder=aac <br> --enable-libfdk-aac             | built-in <br> [libfdk-aac](https://git.code.sf.net/p/opencore-amr/fdk-aac)      |
| Opus                                 | &#10004;  | Audio                                    | -–enable-libopus                                                                    | [libopus](https://github.com/xiph/opus.git)                                     |
| Game Music Emu                       |           | Audio                                    | --enable-libgme                                                                     | [libgme](https://github.com/mcfiredrill/libgme)                                 |
| GSM 06.10                            |           | Audio                                    | --enable-libgsm                                                                     | [libgsm](https://quut.com/gsm/)                                                 |
| Adaptive Multi Rate (AMR)            |           | Audio                                    | --enable-libopencore-amrnb <br> --enable-libopencore-amrnb                          | [libopencore-amr](https://github.com/BelledonneCommunications/opencore-amr.git) |
| OpenMPT                              |           | Audio                                    | --enable-libopenmpt                                                                 | [libopenmpt](https://lib.openmpt.org/libopenmpt/)                               |
| RubberBand                           |           | Audio                                    | --enable-librubberband                                                              | [librubberband](https://github.com/breakfastquay/rubberband.git)                |
| Vorbis                               |           | Audio                                    | --enable-libvorbis                                                                  | [libvorbis](https://gitlab.xiph.org/xiph/vorbis.git)                            |
|                                      |           |                                          |                                                                                     |                                                                                 |
| zimg                                 |           | Image                                    | --enable-libzimg                                                                    | [libzimg](https://github.com/sekrit-twc/zimg.git)                               |
|                                      |           |                                          |                                                                                     |                                                                                 |
| Freetype                             |           | Text                                     | --enable-libfreetype                                                                | [libfreetype](https://download.savannah.gnu.org/releases/freetype/)             |
|                                      |           |                                          |                                                                                     |                                                                                 |
| ASS / SSA                            | &#10004;  | Subtitle                                 | --enable-libass                                                                     | [libass](https://github.com/libass/libass.git)                                  |
|                                      |           |                                          |                                                                                     |                                                                                 |
| Unicode Bidirectional Algorithm      |           | Algorithm                                | --enable-libfribidi                                                                 | [libfribidi](https://github.com/fribidi/fribidi.git)                            |
|                                      |           |                                          |                                                                                     |                                                                                 |
| Secure Reliable Transport            |           | Protocol                                 | --enable-libsrt                                                                     | [libsrt](https://github.com/hwangsaeul/libsrt.git)                              |
| SSH                                  |           | Protocol                                 | --enable-libssh                                                                     | [libssh](https://github.com/CanonicalLtd/libssh.git)                            |
| ZeroMQ                               |           | Protocol                                 | --enable-libzmq                                                                     | [libzmq](https://github.com/zeromq/libzmq.git)                                  |
|                                      |           |                                          |                                                                                     |                                                                                 |
| AMF                                  |           | HWAccel / AMD        / Encoder /         |                                                                                     |                                                                                 |
| NVENC / NVDEC / CUVID                | &#10004;  | HWAccel / NVIDIA     / Encoder / Decoder | --disable-nvenc [autodetect] <br> --disable-nvdec <br> --disable-cuvid [autodetect] |                                                                                 |
| Direct3D 11                          | &#10004;  | HWAccel / Microsoft  /         / Decoder | --disable-d3d11va [autodetect]                                                      |                                                                                 |
| Direct3D 9 / DXVA2                   |           | HWAccel / Microsoft  /         / Decoder | --disable-dxva2 [autodetect]                                                        |                                                                                 |
| Intel MediaSDK                       |           | HWAccel / Intel      / Encoder / Decoder | --enable-libmfx                                                                     | [MediaSDK](https://github.com/Intel-Media-SDK/MediaSDK.git)                     |
| oneVPL                               |           | HWAccel / Intel      / Encoder / Decoder | --enable-libvpl                                                                     | [oneVPL](https://github.com/oneapi-src/oneVPL.git)                              |
| Vulkan                               |           | HWAccel              /         / Decoder | --disable-vulkan [autodetect]                                                       |                                                                                 |
| Media Foundation                     |           |                                          | --enable-mediafoundation [autodetect]                                               |                                                                                 |
|                                      |           |                                          |                                                                                     |                                                                                 |
| dshow                                |           | Device                                   | --enable-indev=dshow                                                                |                                                                                 |
| gdigrab                              |           | Device                                   | --enable-indev=gdigrab                                                              |                                                                                 |
|                                      |           |                                          |                                                                                     |                                                                                 |
| split                                |           | Video / Filter                           | --enable-filter=split                                                               | built-in                                                                        |
| palettegen                           |           | GIF   / Filter                           | --enable-filter=palettegen                                                          | built-in                                                                        |
| paletteuse                           |           | GIF   / Filter                           | --enable-filter=paletteuse                                                          | built-in                                                                        |
| amix                                 |           | Audio / Filter                           | --enable-filter=amix                                                                | built-in                                                                        |
| aresample                            |           | Audio / Filter                           | --enable-filter=aresample                                                           | built-in                                                                        |

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
    --disable-doc \
    --disable-programs \
    --disable-schannel \
    --disable-everything \
    --enable-indev=dshow \
    --enable-libx264 \
    --enable-muxer=gif \
    --enable-muxer=mp4 \
    --enable-muxer=matroska \
    --enable-encoder=gif \
    --enable-encoder=libx264 \
    --enable-encoder=aac \
    --enable-filter=palettegen \
    --enable-filter=paletteuse \
    --enable-filter=vflip \
    --enable-filter=split \
    --enable-filter=scale \
    --enable-filter=amix \
    --enable-filter=aresample \
    --enable-protocol=file \
    --enable-d3d11va \
    --extra-version="cropping_build-ffiirree"


make -j 16
make install
```

### References

1. [Compiling FFmpeg with X264 on Windows 10 using MSVC](https://www.roxlu.com/2019/062/compiling-ffmpeg-with-x264-on-windows-10-using-msvc)
2. [Using FFmpeg with NVIDIA GPU Hardware Acceleration](https://docs.nvidia.com/video-technologies/video-codec-sdk/11.1/ffmpeg-with-nvidia-gpu/index.html)
3. FFmpeg options

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
libx264                 mediafoundation         schannel

External libraries providing hardware acceleration:
d3d11va                 dxva2

Libraries:
avcodec                 avfilter                avutil                  swresample
avdevice                avformat                postproc                swscale

Programs:
ffmpeg                  ffprobe

Enabled decoders:
aac                     ansi                    dsicinaudio             indeo3                  nuv                     ra_288                  v210x
aac_fixed               anull                   dsicinvideo             indeo4                  on2avc                  ralf                    v308
aac_latm                apac                    dss_sp                  indeo5                  opus                    rawvideo                v408
aasc                    ape                     dst                     interplay_acm           paf_audio               realtext                v410
ac3                     aptx                    dvaudio                 interplay_dpcm          paf_video               rka                     vb
ac3_fixed               aptx_hd                 dvbsub                  interplay_video         pam                     rl2                     vble
acelp_kelvin            arbc                    dvdsub                  ipu                     pbm                     roq                     vbn
adpcm_4xm               argo                    dvvideo                 jacosub                 pcm_alaw                roq_dpcm                vc1
adpcm_adx               ass                     dxtory                  jpeg2000                pcm_bluray              rpza                    vc1image
adpcm_afc               asv1                    dxv                     jpegls                  pcm_dvd                 rtv1                    vcr1
adpcm_agm               asv2                    eac3                    jv                      pcm_f16le               rv10                    vmdaudio
adpcm_aica              atrac1                  eacmv                   kgv1                    pcm_f24le               rv20                    vmdvideo
adpcm_argo              atrac3                  eamad                   kmvc                    pcm_f32be               rv30                    vmnc
adpcm_ct                atrac3al                eatgq                   lagarith                pcm_f32le               rv40                    vnull
adpcm_dtk               atrac3p                 eatgv                   loco                    pcm_f64be               s302m                   vorbis
adpcm_ea                atrac3pal               eatqi                   m101                    pcm_f64le               sami                    vp3
adpcm_ea_maxis_xa       atrac9                  eightbps                mace3                   pcm_lxf                 sanm                    vp4
adpcm_ea_r1             aura                    eightsvx_exp            mace6                   pcm_mulaw               sbc                     vp5
adpcm_ea_r2             aura2                   eightsvx_fib            magicyuv                pcm_s16be               scpr                    vp6
adpcm_ea_r3             av1                     escape124               mdec                    pcm_s16be_planar        sdx2_dpcm               vp6a
adpcm_ea_xas            avrn                    escape130               media100                pcm_s16le               sga                     vp6f
adpcm_g722              avrp                    evrc                    metasound               pcm_s16le_planar        sgi                     vp7
adpcm_g726              avs                     fastaudio               microdvd                pcm_s24be               sgirle                  vp8
adpcm_g726le            avui                    ffv1                    mimic                   pcm_s24daud             sheervideo              vp9
adpcm_ima_acorn         ayuv                    ffvhuff                 misc4                   pcm_s24le               shorten                 vplayer
adpcm_ima_alp           bethsoftvid             ffwavesynth             mjpeg                   pcm_s24le_planar        simbiosis_imx           vqa
adpcm_ima_amv           bfi                     fic                     mjpegb                  pcm_s32be               sipr                    vqc
adpcm_ima_apc           bink                    fits                    mlp                     pcm_s32le               siren                   wady_dpcm
adpcm_ima_apm           binkaudio_dct           flac                    mmvideo                 pcm_s32le_planar        smackaud                wavarc
adpcm_ima_cunning       binkaudio_rdft          flic                    mobiclip                pcm_s64be               smacker                 wavpack
adpcm_ima_dat4          bintext                 flv                     motionpixels            pcm_s64le               smc                     wbmp
adpcm_ima_dk3           bitpacked               fmvc                    movtext                 pcm_s8                  smvjpeg                 webp
adpcm_ima_dk4           bmp                     fourxm                  mp1                     pcm_s8_planar           snow                    webvtt
adpcm_ima_ea_eacs       bmv_audio               fraps                   mp1float                pcm_sga                 sol_dpcm                wmalossless
adpcm_ima_ea_sead       bmv_video               frwu                    mp2                     pcm_u16be               sonic                   wmapro
adpcm_ima_iss           bonk                    ftr                     mp2float                pcm_u16le               sp5x                    wmav1
adpcm_ima_moflex        brender_pix             g723_1                  mp3                     pcm_u24be               speedhq                 wmav2
adpcm_ima_mtf           c93                     g729                    mp3adu                  pcm_u24le               speex                   wmavoice
adpcm_ima_oki           cavs                    gdv                     mp3adufloat             pcm_u32be               srt                     wmv1
adpcm_ima_qt            cbd2_dpcm               gem                     mp3float                pcm_u32le               ssa                     wmv2
adpcm_ima_rad           ccaption                gif                     mp3on4                  pcm_u8                  stl                     wmv3
adpcm_ima_smjpeg        cdgraphics              gremlin_dpcm            mp3on4float             pcm_vidc                subrip                  wmv3image
adpcm_ima_ssi           cdtoons                 gsm                     mpc7                    pcx                     subviewer               wnv1
adpcm_ima_wav           cdxl                    gsm_ms                  mpc8                    pfm                     subviewer1              wrapped_avframe
adpcm_ima_ws            cfhd                    h261                    mpeg1video              pgm                     sunrast                 ws_snd1
adpcm_ms                cinepak                 h263                    mpeg2video              pgmyuv                  svq1                    xan_dpcm
adpcm_mtaf              clearvideo              h263i                   mpeg4                   pgssub                  svq3                    xan_wc3
adpcm_psx               cljr                    h263p                   mpegvideo               pgx                     tak                     xan_wc4
adpcm_sbpro_2           cllc                    h264                    mpl2                    phm                     targa                   xbin
adpcm_sbpro_3           comfortnoise            hap                     msa1                    photocd                 targa_y216              xbm
adpcm_sbpro_4           cook                    hca                     msmpeg4v1               pictor                  text                    xface
adpcm_swf               cpia                    hcom                    msmpeg4v2               pixlet                  theora                  xl
adpcm_thp               cri                     hdr                     msmpeg4v3               pjs                     thp                     xma1
adpcm_thp_le            cscd                    hevc                    msnsiren                ppm                     tiertexseqvideo         xma2
adpcm_vima              cyuv                    hnm4_video              msp2                    prores                  tiff                    xpm
adpcm_xa                dca                     hq_hqa                  msrle                   prosumer                tmv                     xsub
adpcm_xmd               dds                     hqx                     mss1                    psd                     truehd                  xwd
adpcm_yamaha            derf_dpcm               huffyuv                 mss2                    ptx                     truemotion1             y41p
adpcm_zork              dfa                     hymt                    msvideo1                qcelp                   truemotion2             ylc
agm                     dfpwm                   iac                     mszh                    qdm2                    truemotion2rt           yop
aic                     dirac                   idcin                   mts2                    qdmc                    truespeech              yuv4
alac                    dnxhd                   idf                     mv30                    qdraw                   tscc2                   zero12v
alias_pix               dolby_e                 iff_ilbm                mvc1                    qoi                     tta
als                     dpx                     ilbc                    mvc2                    qpeg                    twinvq
amrnb                   dsd_lsbf                imc                     mvdv                    qtrle                   txd
amrwb                   dsd_lsbf_planar         imm4                    mxpeg                   r10k                    ulti
amv                     dsd_msbf                imm5                    nellymoser              r210                    utvideo
anm                     dsd_msbf_planar         indeo2                  notchlc                 ra_144                  v210

Enabled encoders:
a64multi                aptx                    flv                     msmpeg4v3               pcm_s64le               roq                     v308
a64multi5               aptx_hd                 g723_1                  msvideo1                pcm_s8                  roq_dpcm                v408
aac                     ass                     gif                     nellymoser              pcm_s8_planar           rpza                    v410
aac_mf                  asv1                    h261                    opus                    pcm_u16be               rv10                    vbn
ac3                     asv2                    h263                    pam                     pcm_u16le               rv20                    vc2
ac3_fixed               avrp                    h263p                   pbm                     pcm_u24be               s302m                   vnull
ac3_mf                  avui                    h264_mf                 pcm_alaw                pcm_u24le               sbc                     vorbis
adpcm_adx               ayuv                    hdr                     pcm_bluray              pcm_u32be               sgi                     wavpack
adpcm_argo              bitpacked               hevc_mf                 pcm_dvd                 pcm_u32le               smc                     wbmp
adpcm_g722              bmp                     huffyuv                 pcm_f32be               pcm_u8                  snow                    webvtt
adpcm_g726              cfhd                    jpeg2000                pcm_f32le               pcm_vidc                sonic                   wmav1
adpcm_g726le            cinepak                 jpegls                  pcm_f64be               pcx                     sonic_ls                wmav2
adpcm_ima_alp           cljr                    libx264                 pcm_f64le               pfm                     speedhq                 wmv1
adpcm_ima_amv           comfortnoise            libx264rgb              pcm_mulaw               pgm                     srt                     wmv2
adpcm_ima_apm           dca                     ljpeg                   pcm_s16be               pgmyuv                  ssa                     wrapped_avframe
adpcm_ima_qt            dfpwm                   magicyuv                pcm_s16be_planar        phm                     subrip                  xbm
adpcm_ima_ssi           dnxhd                   mjpeg                   pcm_s16le               ppm                     sunrast                 xface
adpcm_ima_wav           dpx                     mlp                     pcm_s16le_planar        prores                  svq1                    xsub
adpcm_ima_ws            dvbsub                  movtext                 pcm_s24be               prores_aw               targa                   xwd
adpcm_ms                dvdsub                  mp2                     pcm_s24daud             prores_ks               text                    y41p
adpcm_swf               dvvideo                 mp2fixed                pcm_s24le               qoi                     tiff                    yuv4
adpcm_yamaha            eac3                    mp3_mf                  pcm_s24le_planar        qtrle                   truehd
alac                    ffv1                    mpeg1video              pcm_s32be               r10k                    tta
alias_pix               ffvhuff                 mpeg2video              pcm_s32le               r210                    ttml
amv                     fits                    mpeg4                   pcm_s32le_planar        ra_144                  utvideo
anull                   flac                    msmpeg4v2               pcm_s64be               rawvideo                v210

Enabled hwaccels:
av1_d3d11va             h264_d3d11va            hevc_d3d11va            mpeg2_d3d11va           vc1_d3d11va             vp9_d3d11va             wmv3_d3d11va
av1_d3d11va2            h264_d3d11va2           hevc_d3d11va2           mpeg2_d3d11va2          vc1_d3d11va2            vp9_d3d11va2            wmv3_d3d11va2
av1_dxva2               h264_dxva2              hevc_dxva2              mpeg2_dxva2             vc1_dxva2               vp9_dxva2               wmv3_dxva2

Enabled parsers:
aac                     cavsvideo               dvbsub                  h261                    mlp                     rv40                    webp
aac_latm                cook                    dvd_nav                 h263                    mpeg4video              sbc                     xbm
ac3                     cri                     dvdsub                  h264                    mpegaudio               sipr                    xma
adx                     dca                     flac                    hdr                     mpegvideo               tak                     xwd
amr                     dirac                   ftr                     hevc                    opus                    vc1
av1                     dnxhd                   g723_1                  ipu                     png                     vorbis
avs2                    dolby_e                 g729                    jpeg2000                pnm                     vp3
avs3                    dpx                     gif                     misc4                   qoi                     vp8
bmp                     dvaudio                 gsm                     mjpeg                   rv30                    vp9

Enabled demuxers:
aa                      bmv                     gdv                     image_sgi_pipe          mpegtsraw               pva                     tak
aac                     boa                     genh                    image_sunrast_pipe      mpegvideo               pvf                     tedcaptions
aax                     bonk                    gif                     image_svg_pipe          mpjpeg                  qcp                     thp
ac3                     brstm                   gsm                     image_tiff_pipe         mpl2                    r3d                     threedostr
ace                     c93                     gxf                     image_vbn_pipe          mpsub                   rawvideo                tiertexseq
acm                     caf                     h261                    image_webp_pipe         msf                     realtext                tmv
act                     cavsvideo               h263                    image_xbm_pipe          msnwc_tcp               redspark                truehd
adf                     cdg                     h264                    image_xpm_pipe          msp                     rka                     tta
adp                     cdxl                    hca                     image_xwd_pipe          mtaf                    rl2                     tty
ads                     cine                    hcom                    ingenient               mtv                     rm                      txd
adx                     codec2                  hevc                    ipmovie                 musx                    roq                     ty
aea                     codec2raw               hls                     ipu                     mv                      rpl                     v210
afc                     concat                  hnm                     ircam                   mvi                     rsd                     v210x
aiff                    data                    ico                     iss                     mxf                     rso                     vag
aix                     daud                    idcin                   iv8                     mxg                     rtp                     vc1
alp                     dcstr                   idf                     ivf                     nc                      rtsp                    vc1t
amr                     derf                    iff                     ivr                     nistsphere              s337m                   vividas
amrnb                   dfa                     ifv                     jacosub                 nsp                     sami                    vivo
amrwb                   dfpwm                   ilbc                    jpegxl_anim             nsv                     sap                     vmd
anm                     dhav                    image2                  jv                      nut                     sbc                     vobsub
apac                    dirac                   image2_alias_pix        kux                     nuv                     sbg                     voc
apc                     dnxhd                   image2_brender_pix      kvag                    obu                     scc                     vpk
ape                     dsf                     image2pipe              laf                     ogg                     scd                     vplayer
apm                     dsicin                  image_bmp_pipe          live_flv                oma                     sdns                    vqf
apng                    dss                     image_cri_pipe          lmlm4                   paf                     sdp                     w64
aptx                    dts                     image_dds_pipe          loas                    pcm_alaw                sdr2                    wady
aptx_hd                 dtshd                   image_dpx_pipe          lrc                     pcm_f32be               sds                     wav
aqtitle                 dv                      image_exr_pipe          luodat                  pcm_f32le               sdx                     wavarc
argo_asf                dvbsub                  image_gem_pipe          lvf                     pcm_f64be               segafilm                wc3
argo_brp                dvbtxt                  image_gif_pipe          lxf                     pcm_f64le               ser                     webm_dash_manifest
argo_cvg                dxa                     image_hdr_pipe          m4v                     pcm_mulaw               sga                     webvtt
asf                     ea                      image_j2k_pipe          matroska                pcm_s16be               shorten                 wsaud
asf_o                   ea_cdata                image_jpeg_pipe         mca                     pcm_s16le               siff                    wsd
ass                     eac3                    image_jpegls_pipe       mcc                     pcm_s24be               simbiosis_imx           wsvqa
ast                     epaf                    image_jpegxl_pipe       mgsts                   pcm_s24le               sln                     wtv
au                      ffmetadata              image_pam_pipe          microdvd                pcm_s32be               smacker                 wv
av1                     filmstrip               image_pbm_pipe          mjpeg                   pcm_s32le               smjpeg                  wve
avi                     fits                    image_pcx_pipe          mjpeg_2000              pcm_s8                  smush                   xa
avr                     flac                    image_pfm_pipe          mlp                     pcm_u16be               sol                     xbin
avs                     flic                    image_pgm_pipe          mlv                     pcm_u16le               sox                     xmd
avs2                    flv                     image_pgmyuv_pipe       mm                      pcm_u24be               spdif                   xmv
avs3                    fourxm                  image_pgx_pipe          mmf                     pcm_u24le               srt                     xvag
bethsoftvid             frm                     image_phm_pipe          mods                    pcm_u32be               stl                     xwma
bfi                     fsb                     image_photocd_pipe      moflex                  pcm_u32le               str                     yop
bfstm                   fwse                    image_pictor_pipe       mov                     pcm_u8                  subviewer               yuv4mpegpipe
bink                    g722                    image_png_pipe          mp3                     pcm_vidc                subviewer1
binka                   g723_1                  image_ppm_pipe          mpc                     pdv                     sup
bintext                 g726                    image_psd_pipe          mpc8                    pjs                     svag
bit                     g726le                  image_qdraw_pipe        mpegps                  pmp                     svs
bitpacked               g729                    image_qoi_pipe          mpegts                  pp_bnk                  swf

Enabled muxers:
a64                     caf                     g722                    lrc                     mxf_opatom              pcm_u24le               streamhash
ac3                     cavsvideo               g723_1                  m4v                     null                    pcm_u32be               sup
adts                    codec2                  g726                    matroska                nut                     pcm_u32le               swf
adx                     codec2raw               g726le                  matroska_audio          obu                     pcm_u8                  tee
aiff                    crc                     gif                     md5                     oga                     pcm_vidc                tg2
alp                     dash                    gsm                     microdvd                ogg                     psp                     tgp
amr                     data                    gxf                     mjpeg                   ogv                     rawvideo                truehd
amv                     daud                    h261                    mkvtimestamp_v2         oma                     rm                      tta
apm                     dfpwm                   h263                    mlp                     opus                    roq                     ttml
apng                    dirac                   h264                    mmf                     pcm_alaw                rso                     uncodedframecrc
aptx                    dnxhd                   hash                    mov                     pcm_f32be               rtp                     vc1
aptx_hd                 dts                     hds                     mp2                     pcm_f32le               rtp_mpegts              vc1t
argo_asf                dv                      hevc                    mp3                     pcm_f64be               rtsp                    voc
argo_cvg                eac3                    hls                     mp4                     pcm_f64le               sap                     w64
asf                     f4v                     ico                     mpeg1system             pcm_mulaw               sbc                     wav
asf_stream              ffmetadata              ilbc                    mpeg1vcd                pcm_s16be               scc                     webm
ass                     fifo                    image2                  mpeg1video              pcm_s16le               segafilm                webm_chunk
ast                     fifo_test               image2pipe              mpeg2dvd                pcm_s24be               segment                 webm_dash_manifest
au                      filmstrip               ipod                    mpeg2svcd               pcm_s24le               smjpeg                  webp
avi                     fits                    ircam                   mpeg2video              pcm_s32be               smoothstreaming         webvtt
avif                    flac                    ismv                    mpeg2vob                pcm_s32le               sox                     wsaud
avm2                    flv                     ivf                     mpegts                  pcm_s8                  spdif                   wtv
avs2                    framecrc                jacosub                 mpjpeg                  pcm_u16be               spx                     wv
avs3                    framehash               kvag                    mxf                     pcm_u16le               srt                     yuv4mpegpipe
bit                     framemd5                latm                    mxf_d10                 pcm_u24be               stream_segment

Enabled protocols:
async                   fd                      hls                     ipns_gateway            rtmp                    subfile
cache                   ffrtmphttp              http                    md5                     rtmps                   tcp
concat                  file                    httpproxy               mmsh                    rtmpt                   tee
concatf                 ftp                     https                   mmst                    rtmpts                  tls
crypto                  gopher                  icecast                 pipe                    rtp                     udp
data                    gophers                 ipfs_gateway            prompeg                 srtp                    udplite

Enabled filters:
a3dscope                aphaser                 colorchannelmixer       exposure                limitdiff               qp                      ssim
abench                  aphaseshift             colorchart              extractplanes           limiter                 random                  ssim360
abitscope               apsyclip                colorcontrast           extrastereo             loop                    readeia608              stereo3d
acompressor             apulsator               colorcorrect            fade                    loudnorm                readvitc                stereotools
acontrast               arealtime               colorhold               feedback                lowpass                 realtime                stereowiden
acopy                   aresample               colorize                fftdnoiz                lowshelf                remap                   streamselect
acrossfade              areverse                colorkey                fftfilt                 lumakey                 removegrain             super2xsai
acrossover              arls                    colorlevels             field                   lut                     removelogo              superequalizer
acrusher                arnndn                  colormap                fieldhint               lut1d                   repeatfields            surround
acue                    asdr                    colormatrix             fieldmatch              lut2                    replaygain              swaprect
addroi                  asegment                colorspace              fieldorder              lut3d                   reverse                 swapuv
adeclick                aselect                 colorspectrum           fifo                    lutrgb                  rgbashift               tblend
adeclip                 asendcmd                colortemperature        fillborders             lutyuv                  rgbtestsrc              telecine
adecorrelate            asetnsamples            compand                 find_rect               mandelbrot              roberts                 testsrc
adelay                  asetpts                 compensationdelay       firequalizer            maskedclamp             rotate                  testsrc2
adenorm                 asetrate                concat                  flanger                 maskedmax               sab                     thistogram
aderivative             asettb                  convolution             floodfill               maskedmerge             scale                   threshold
adrawgraph              ashowinfo               convolve                format                  maskedmin               scale2ref               thumbnail
adrc                    asidedata               copy                    fps                     maskedthreshold         scdet                   tile
adynamicequalizer       asoftclip               corr                    framepack               maskfun                 scharr                  tiltshelf
adynamicsmooth          aspectralstats          cover_rect              framerate               mcdeint                 scroll                  tinterlace
aecho                   asplit                  crop                    framestep               mcompand                segment                 tlut2
aemphasis               astats                  cropdetect              freezedetect            median                  select                  tmedian
aeval                   astreamselect           crossfeed               freezeframes            mergeplanes             selectivecolor          tmidequalizer
aevalsrc                asubboost               crystalizer             fspp                    mestimate               sendcmd                 tmix
aexciter                asubcut                 cue                     gblur                   metadata                separatefields          tonemap
afade                   asupercut               curves                  geq                     midequalizer            setdar                  tpad
afdelaysrc              asuperpass              datascope               gradfun                 minterpolate            setfield                transpose
afftdn                  asuperstop              dblur                   gradients               mix                     setparams               treble
afftfilt                atadenoise              dcshift                 graphmonitor            monochrome              setpts                  tremolo
afifo                   atempo                  dctdnoiz                grayworld               morpho                  setrange                trim
afir                    atilt                   ddagrab                 greyedge                movie                   setsar                  unpremultiply
afireqsrc               atrim                   deband                  guided                  mpdecimate              settb                   unsharp
afirsrc                 avectorscope            deblock                 haas                    mptestsrc               shear                   untile
aformat                 avgblur                 decimate                haldclut                msad                    showcqt                 uspp
afreqshift              avsynctest              deconvolve              haldclutsrc             multiply                showcwt                 v360
afwtdn                  axcorrelate             dedot                   hdcd                    negate                  showfreqs               vaguedenoiser
agate                   backgroundkey           deesser                 headphone               nlmeans                 showinfo                varblur
agraphmonitor           bandpass                deflate                 hflip                   nnedi                   showpalette             vectorscope
ahistogram              bandreject              deflicker               highpass                noformat                showspatial             vflip
aiir                    bass                    dejudder                highshelf               noise                   showspectrum            vfrdet
aintegral               bbox                    delogo                  hilbert                 normalize               showspectrumpic         vibrance
ainterleave             bench                   derain                  histeq                  null                    showvolume              vibrato
alatency                bilateral               deshake                 histogram               nullsink                showwaves               vif
alimiter                biquad                  despill                 hqdn3d                  nullsrc                 showwavespic            vignette
allpass                 bitplanenoise           detelecine              hqx                     oscilloscope            shuffleframes           virtualbass
allrgb                  blackdetect             dialoguenhance          hstack                  overlay                 shufflepixels           vmafmotion
allyuv                  blackframe              dilation                hsvhold                 owdenoise               shuffleplanes           volume
aloop                   blend                   displace                hsvkey                  pad                     sidechaincompress       volumedetect
alphaextract            blockdetect             dnn_classify            hue                     pal100bars              sidechaingate           vstack
alphamerge              blurdetect              dnn_detect              huesaturation           pal75bars               sidedata                w3fdif
amerge                  bm3d                    dnn_processing          hwdownload              palettegen              sierpinski              waveform
ametadata               boxblur                 doubleweave             hwmap                   paletteuse              signalstats             weave
amix                    bwdif                   drawbox                 hwupload                pan                     signature               xbr
amovie                  cas                     drawgraph               hysteresis              perms                   silencedetect           xcorrelate
amplify                 ccrepack                drawgrid                identity                perspective             silenceremove           xfade
amultiply               cellauto                drmeter                 idet                    phase                   sinc                    xmedian
anequalizer             channelmap              dynaudnorm              il                      photosensitivity        sine                    xstack
anlmdn                  channelsplit            earwax                  inflate                 pixdesctest             siti                    yadif
anlmf                   chorus                  ebur128                 interlace               pixelize                smartblur               yaepblur
anlms                   chromahold              edgedetect              interleave              pixscope                smptebars               yuvtestsrc
anoisesrc               chromakey               elbg                    join                    pp                      smptehdbars             zoneplate
anull                   chromanr                entropy                 kerndeint               pp7                     sobel                   zoompan
anullsink               chromashift             epx                     kirsch                  premultiply             spectrumsynth
anullsrc                ciescope                eq                      lagfun                  prewitt                 speechnorm
apad                    codecview               equalizer               latency                 pseudocolor             split
aperms                  color                   erosion                 lenscorrection          psnr                    spp
aphasemeter             colorbalance            estdif                  life                    pullup                  sr

Enabled bsfs:
aac_adtstoasc           dts2pts                 h264_metadata           imx_dump_header         mpeg2_metadata          pgs_frame_merge         truehd_core
av1_frame_merge         dump_extradata          h264_mp4toannexb        media100_to_mjpegb      mpeg4_unpack_bframes    prores_metadata         vp9_metadata
av1_frame_split         dv_error_marker         h264_redundant_pps      mjpeg2jpeg              noise                   remove_extradata        vp9_raw_reorder
av1_metadata            eac3_core               hapqa_extract           mjpega_dump_header      null                    setts                   vp9_superframe
chomp                   extract_extradata       hevc_metadata           mov2textsub             opus_metadata           text2movsub             vp9_superframe_split
dca_core                filter_units            hevc_mp4toannexb        mp3_header_decompress   pcm_rechunk             trace_headers

Enabled indevs:
dshow                   gdigrab                 lavfi                   vfwcap

Enabled outdevs:

License: GPL version 3 or later


```