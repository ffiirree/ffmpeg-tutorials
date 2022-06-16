# FFmpeg Examples

包括 `解/编码`、`(复杂)滤波器`、`录屏/摄像头`、`音视频播放` 的基础示例以及相关教程。

Examples and tutorials for `decoding/encoding`, `(complex)filtering`, `screen/camera recording`, `audio/video playing`, etc.

## Installation

### Windows

#### FFmpeg

> Version = 4.4.2

- [Windows builds](https://www.ffmpeg.org/download.html#build-windows): 注意下载**库(shared)**版本，添加到**环境变量**中
- [Build from source](/compile_on_windows.md)

#### Qt

> 6.0 > Version >= 5.12.10

- [Download](https://download.qt.io/archive/qt/): 安装后注意添加目录到**环境变量**中

### Ubuntu

```bash
sudo apt install build-essential git cmake

sudo apt install ffmpeg libavfilter-dev libavdevice-dev libavutil-dev libavformat-dev libswresample-dev libswscale-dev

# Ubuntu < 22.04
sudo apt install qt5-default libqt5x11extras5-dev qtmultimedia5-dev 

# Ubuntu 22.04
# Download qt-xxx.run from https://download.qt.io/archive/qt/
chmod +x qt-opensource-linux-x64-5.12.12.run
# Double click qt-opensource-linux-x64-5.12.12.run and install

# for error: GL/gl.h: No such file or directory
sudo apt install mesa-common-dev
```

### C++ & CMake

- C++ 17
- CMake >= 3.12

### Usage

```bash
git clone https://github.com/ffiirree/ffmpeg_examples.git --recursive

# 或者单独获取子模块
git submodule init
git submodule update
```

## Examples

- [x] [remuxing](/01_remuxing/README.md)
- [ ] [transcoding](/02_transcoding/README.md)
- [x] [recording camera / screen / microphone](/03_recording/README.md)
- [ ] [simple filter](/04_simple_filters/README.md)
- [ ] [complex filter](/05_complex_filter/README.md)
- [x] [generating high quality GIF](/06_gen_gif/README.md)
- [ ] [audio player with Qt](/07_audio_player/README.md)
- [ ] [video player with Qt](/08_video_player/README.md)
- [ ] [media player with Qt (syncing audio and video)](/09_media_player/README.md)
- [x] [RTMP Streaming](/10_streaming/README.md)
- [ ] [record speakers/microphones by WASAPI(Windows)](/11_wasapi_capture/README.md)
- [ ] [render audio stream by WASAPI(Windows)](/12_wasapi_render/README.md)


## References

- [FFmpeg](https://ffmpeg.org/)
- [ffmpeg-libav-tutorial](https://github.com/leandromoreira/ffmpeg-libav-tutorial)
- [How to Write a Video Player in Less Than 1000 Lines](http://dranger.com/ffmpeg/)
- [High quality GIF with FFmpeg](http://blog.pkh.me/p/21-high-quality-gif-with-ffmpeg.html)
- [SimplePlayer](https://www.cnblogs.com/TaigaCon/)