# Media Player

## UI

![Player UI](/images/player.png)

## 架构

使用多线程，并进行音视频同步

- `播放器主线程`：UI绘制，绘制`视频帧`
- `文件读取线程`：解封装，读取为`AVPacket`，按照包类型放入对应的队列
- `视频解码线程`：视频解码
- `音频解码线程`：音频解码
- `QAudioOuput`：播放音频

## 音频播放

[Audio Player](/07_audio_player/README.md)

## 视频播放

[Video Player](/08_video_player/README.md)


## 音视频同步