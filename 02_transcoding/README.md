# Transcoding

不同的编码有不同的优点，一般较新的编码格式对原始数据的压缩率越高，最终的封装文件体积越小，但是播放器可能不支持一些新的编码格式，这时候需要对文件进行转码。

## 实现功能等效FFmpeg命令

将`hevc(h265)`格式转为 `h264`编码格式

```bash
ffmpeg -i hevc.mkv -c:v libx264 x264.mp4
```

> 注意：本示例代码中未对解码器和编码器进行清空，实际解码的帧小于实际视频帧。关于这部分处理会在后续涉及。

## 转码

相对于重封装，转码需要对读取的packet进行 *解码* 再 *编码* 的过程。因此，需要为编码和解码过程准备对应的 编码器(encoder) 和解码器(decoder)。

### 常用参数

- `fps`: `AVStream.avg_frame_rate` May be set by libavformat when creating the stream or in avformat_find_stream_info().
- `tbr`: `AVStream.r_frame_rate` Real base framerate of the stream.
- `tbn`: `AVStream.time_base`
- `tbc`: `AVCodecContext.time_base`
- `pix_fmt`: `AVPixelFormat`
- `SAR`: `AVCodecContext.sample_aspect_ratio`


### 帧类型

每一帧`AVFrame`都有一个类型`pict_type`， 比如`h264`和`h265`常见的 `I` / `B` / `P`帧。
解码后的帧会带有原编码时的帧类型，带有类型的帧在重新编码时，类型信息会影响编码器的行为，因此在重新编码时，把帧类型抹掉，让编码器完全自动处理。

```c
enum AVPictureType {
    AV_PICTURE_TYPE_NONE = 0, ///< Undefined
    AV_PICTURE_TYPE_I,     ///< Intra
    AV_PICTURE_TYPE_P,     ///< Predicted
    AV_PICTURE_TYPE_B,     ///< Bi-dir predicted
    AV_PICTURE_TYPE_S,     ///< S(GMC)-VOP MPEG-4
    AV_PICTURE_TYPE_SI,    ///< Switching Intra
    AV_PICTURE_TYPE_SP,    ///< Switching Predicted
    AV_PICTURE_TYPE_BI,    ///< BI type
};
```

抹掉帧的类型信息：

```c
in_frame->pict_type = AV_PICTURE_TYPE_NONE;
```