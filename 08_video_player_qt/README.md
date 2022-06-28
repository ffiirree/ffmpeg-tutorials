# Video Player with Qt

本示例使用 QWidget 控件渲染视频帧，QWidget 只能渲染RGB的QImage，如果想要使用OpenGL加速直接渲染
YUV视频帧，可以使用`QOpenGLWidget`并写好YUV的渲染流程，或者用[SDL渲染YUV视频](/08_video_player_sdl/README.md)。

## 架构设计

本示例使用两个线程：**主线程**作为Qt界面绘制线程 和 **解码线程**进行视频解码并按播放速度调用回调函数。

### 解码线程

解码线程 的解码流程和之前完全相同，由于QWidget只能渲染RGB视频帧，因此需要将解码的YUV格式转换为RGB格式，这里使用了filter进行自动转换。

解码线程使用视频帧的pts和av_usleep控制回调的速度，即视频帧的实际渲染时间。


### 主线程

主线程主要进行界面绘制，当解码线程调用回调函数时，需要取出新的一帧视频并发送界面更新事件。

```C++
// frame_在主线程读，在解码线程写，需要加锁
mtx_.lock();
if(frame_) {
    // 将新视频帧内存的所有权转移到frame_
    av_frame_unref(frame_);
    av_frame_ref(frame_, frame);
}
mtx_.unlock();

// 发送Qt界面更新事件
QWidget::update();
```

此后在`paintEvent`函数中进行绘制更新即可。