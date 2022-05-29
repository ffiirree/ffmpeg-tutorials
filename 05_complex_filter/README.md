# Complex filter

使用复杂滤波器为视频添加图片水印。 将图片

![watermark](../images/watermark.png)

作为水印添加到视频的左上角，效果如下

![marked](../images/marked.png).

等效FFmpeg命令：
```bash
ffmpeg -hide_banner -i .\images\watermark.png -i .\hevc.mkv -filter_complex "[0:v] scale=64:-1:flags=lanczos,vflip [s];[1:v][s]overlay=10:10" out.mp4
```

## 图片解码

png图片的解码和视频解码完全一样，但是只有1帧，因此要正确获取解码器中缓存的帧。

## 复杂滤波器

多个输入的复杂滤波器需要判定根据哪个输入流结束后退出。
