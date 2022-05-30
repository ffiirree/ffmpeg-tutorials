# Recording

屏幕/摄像头录制是一个常见的需求。相较于转码示例，这个示例主要不同在于：

- 打开并访问输入设备
- 使用`swscale`对解码图片进行缩放和格式转换
- 正确处理编解码帧的时间戳

## Windows

```bash
# 列出所有设备
ffmpeg -hide_banner -f dshow -list_devices true -i dummy

# 录制摄像头
ffmpeg -f dshow -i video="HD WebCam" -c:v libx264 camera.mp4
# 录制屏幕
ffmpeg -f gdigrab -framerate 25 -offset_x 100 -offset_y 200 -video_size 720x360 -i desktop -c:v libx264 screen.mp4
```

## Linux

```bash
# 录制摄像头
ffmpeg -f v4l2 -i /dev/video0 -c:v libx264 camera.mkv
# 录制屏幕
ffmpeg -framerate 25 -video_size 720x360 -f x11grab -i :0.0+100,200 -c:v libx264 screen.mp4
```

## Hard way

### 打开输入设备

相较于打开视频文件，访问其他输入设备需要多一点准备，首先是

```c
avdevice_register_all();
```

在打开设备的时候，需要指定输入设备的`format`:

```c
// windows和linux上的输入设备格式不同，在ffmpeg命令中使用-f指定，参看本节开头的ffmpeg命令
avformat_open_input(&decoder_fmt_ctx, input, av_find_input_format(input_format), nullptr)
```

### 缩放和`PIX_FMT_`转换

准备上下文，以及为转换后的图片分配空间

```c
    SwsContext * sws_ctx = sws_getContext(
            decoder_ctx->width,decoder_ctx->height,decoder_ctx->pix_fmt,
            encoder_ctx->width,encoder_ctx->height,encoder_ctx->pix_fmt,
            SWS_BICUBIC, nullptr, nullptr, nullptr
    );

    AVFrame * scaled_frame = av_frame_alloc();
    if(av_image_alloc(scaled_frame->data, scaled_frame->linesize,
                      encoder_ctx->width, encoder_ctx->height,
                      encoder_ctx->pix_fmt, 8) < 0) {
        fprintf(stderr, "av_image_alloc()\n");
        return -1;
    }
```

缩放和转换调用：

```c
sws_scale(
    sws_ctx,
    static_cast<const uint8_t *const *>(decoded_frame->data), decoded_frame->linesize,
    0, decoder_ctx->height,
    scaled_frame->data, scaled_frame->linesize);
```

注意要对转换后的帧设定`height`和`width`等基础信息。

> 缩放和格式转换也可以用filter实现，而且filter会自动进行格式协商，见后续filter等示例。

### 时间设定

因为视频是由不连续的帧组成的，因此需要`帧率`和其他时间戳来控制实际每一帧的显示时间。

首先说一下`AVRational`结构体，它在ffmpeg中用来表示有理数，且用的是分数`num/dem`的形式。

```c
/**
 * Rational number (pair of numerator and denominator).
 */
typedef struct AVRational{
    int num; ///< Numerator
    int den; ///< Denominator
} AVRational;
```

在ffmpeg中，帧率`framerate`和时间基数`time_base`都是用`AVRational`表示的。

- `framerate`: 帧率，例如24帧表示为`AVRational{24, 1}`
- `time_base`: 时间戳单位(或者理解为时间片，是对1s的缩放)，ffmpeg中的时间单位并不是固定的1s/1ms/1us等，而是`解码器(AVCodecContext)`和`每条视频流/音频流(AVStream)`可以设定各自的`time_base`。如果`time_base`是1ms，则为`AVRational{1, 1000}`，即1s的1000分之一。
  - `AVCodecContext.time_base` gives the exact fps. If ticks_per_frame is 2, downsize the time_base with 1/2. For example, if AVCodecContext.time_base (1, 60) and ticks_per_frame is 1, the fps is 60. If ticks_per_frame is 2, fps is 30. 也就是`AVCodecContext.time_base`和`fps`是相关的。 `fps`固定时，`AVCodecContex.time_base`应该为`1/framerate`；`fps`不固定时，那就没有`fps`这个概念了(或者说可以认为是`1/AVCodecContex.time_base`)
  - The time_base for AVStream is only for time unit in the methods in AVStream, such as getting the time of one frame, or the .start variable. 也就是`AVStream.time_base`只是一个精确的时间单位就可以了，而且编码时`AVStream.time_base`在手动设定后，可能会被ffmpeg根据编码格式重新设定。
  - 关于为什么要有`AVCodecContext.time_base`和`AVStream.time_base`两种: Generaly coder time base is inverse Frame Rate, so we can increment PTS simple by 1 for next frame, but Stream time base can depend on some format/codec specifications. Packets PTS/DTS must be in Stream time-base units before writing so rescaling between coder and stream time bases is required.
- `pts`: `presentation timestamp`，也就是`显示时间`，用来指定该帧播放的时间。`pts`的时间单位就是`time_base`，也就是从视频开始到这一帧经过了多少个`time_base`。
  - `AVPacket.pts` 的单位必须是对应流的`AVStream.time_base`
  - `AVFrame.pts` 的单位则不确定，是 解码/编码 时输入的 packet 或 frame 对应的time_base (TODO: 这个存疑)
- `dts`: `decompression timestamp`，即`解码时间`。由于有些编码格式由预测帧等帧的存在，帧的解码顺序和编码顺序不同，`pts >= dts`。
- `duration`: 两帧之间的间隔。
- `AVFormatContext.r_frame_rate`: libavformats猜的framerate

  
### 设定`time_base`和`pts`

编码时的`time_base`需要手动设定：

```c
// 编码器和每条视频都有time_base，这里设定为相同
encoder_ctx->time_base = decoder_fmt_ctx->streams[video_stream_idx]->time_base;
// 视频流的time_base要在调用avformat_write_header()之前设定(或者不设定)，且调用`avformat_write_header()`后，流的time_base会被覆写，因此不一定是设定时的值
encoder_fmt_ctx->streams[0]->time_base = encoder_ctx->time_base;
```

视频转码时，解码的帧是有对应的`pts`等时间戳的，但是 录屏/录制摄像头等视频流没有正确的时间戳，我们需要通过系统时钟计算并设定每一帧的时间戳。

纪录开始录制的时间`first_pts`，然后用`av_gettime_relative()`获取当前时间，减去`first_pts`作为帧的`pts`。ffmpeg内部的时间单位是`AVRational{1, 1000000}`，需要转换到对应的时间单位上。

```c
int64_t first_pts = AV_NOPTS_VALUE;
first_pts = first_pts == AV_NOPTS_VALUE ? av_gettime_relative() : first_pts;
scaled_frame->pts = av_rescale_q(av_gettime_relative() - first_pts, { 1, AV_TIME_BASE }, encoder_fmt_ctx->streams[0]->time_base);
```

## References

- [[Ffmpeg-devel] Frame rates and time_base](http://ffmpeg.org/pipermail/ffmpeg-devel/2005-May/003079.html)
- [ffmpeg time unit explanation and av_seek_frame method](https://stackoverflow.com/questions/12234949/ffmpeg-time-unit-explanation-and-av-seek-frame-method)
- [[Libav-user] Helo in understanding PTS and DTS](https://ffmpeg.org/pipermail/libav-user/2012-November/003207.html)