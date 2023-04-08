# FFmpeg Examples

包括 `解 / 编码`、`(复杂)滤波器`、`屏幕 / 摄像头 / 音频捕获`、`音视频播放` 的基础示例以及相关教程。

Examples and tutorials for `decoding / encoding`, `(complex) filtering`, `screen, camera, and audio capture`, `audio / video playing`, etc.

## Examples

- [x] [remuxing](/01_remuxing/README.md)
- [ ] [transcoding](/02_transcoding/README.md)
- [x] [camera / screen / microphone recording](/03_recording/README.md)
- [ ] [simple filter](/04_simple_filters/README.md)
- [ ] [complex filter](/05_complex_filter/README.md)
- [x] [generating high quality GIF](/06_gen_gif/README.md)
- [ ] [audio player with Qt](/07_audio_player/README.md)
- [x] [video player with Qt](/08_video_player_qt/README.md)
- [ ] [media player with Qt (syncing audio and video)](/09_media_player/README.md)
- [x] [RTMP Streaming](/10_streaming/README.md)
- [ ] [hardware acceleration](/13_hwaccel/README.md)

### Platform Specific

- [ ] Windows - [WASAPI : audio capture](/11_wasapi_capture/README.md)
- [ ] Windows - [WASAPI : audio rendering](/12_wasapi_render/README.md)
- [ ] Windows - [Windows Graphics Capture : screen capture](/14_windows_wgc/README.md)
- [ ] Linux - [PulseAudio : audio capture](/15_linux_pulse/README.md)
- [ ] Linux - [V4L2 : camera capture](/16_linux_v4l2/README.md)
- [ ] Windows - [DirectShow : camera capture](/17_win_dshow/README.md)

## Installation

### Windows

`Windows Graphics Capture` 示例需要较高的Windows版本:

- `Windows Version >= 10.0.17134.48` (aka `Windows 10 Version 1803`)
- `Windows SDK Version >= 10.0.20348.0`

如果你的Windows或SDK不能满足要求，你可以选择:

- 通过`-DDISABLE_WGC=ON`来禁用 `Windows Graphics Capture` 示例
- 通过 `Visual Studio Installer` 安装最新的 `Windows SDK`

#### FFmpeg

> Version : 4.4.x or 5.1.x

- [Windows builds](https://www.ffmpeg.org/download.html#build-windows): 注意下载`shared`版本，添加到**环境变量**中
- [Build from source](/compile_on_windows.md)

#### Qt5

> 6.0 > Version >= 5.12

- [Download](https://download.qt.io/archive/qt/): 安装后注意添加目录到**环境变量**中

### Ubuntu

```bash
sudo apt install build-essential git cmake

sudo apt install ffmpeg libavfilter-dev libavdevice-dev libavutil-dev libavformat-dev libswresample-dev libswscale-dev

# Ubuntu 20.04 
sudo apt install qt5-default libqt5x11extras5-dev qtmultimedia5-dev
# Ubuntu 22.04
sudo apt install qtbase5-dev libqt5x11extras5-dev qtmultimedia5-dev

# pulse
sudo apt install libpulse-dev

# v4l2
sudo apt install libv4l-dev v4l-utils

# for error: GL/gl.h: No such file or directory
sudo apt install mesa-common-dev
```

### C++ & CMake

- C++ 20
- CMake >= 3.16

### Other dependencies

> No installation required

- [glog](https://github.com/google/glog): Google Logging (glog) is a C++98 library that implements application-level logging. The library provides logging APIs based on C++-style streams and various helper macros.
- [fmt](https://github.com/fmtlib/fmt): {fmt} is an open-source formatting library providing a fast and safe alternative to C stdio and C++ iostreams.

### Usage

```bash
git clone https://github.com/ffiirree/ffmpeg_examples.git --recursive

# 或者单独获取子模块
git submodule init
git submodule update
```

## References

- [FFmpeg](https://ffmpeg.org/)
- [ffmpeg-libav-tutorial](https://github.com/leandromoreira/ffmpeg-libav-tutorial)
- [How to Write a Video Player in Less Than 1000 Lines](http://dranger.com/ffmpeg/)
- [High quality GIF with FFmpeg](http://blog.pkh.me/p/21-high-quality-gif-with-ffmpeg.html)
- [SimplePlayer](https://www.cnblogs.com/TaigaCon/)