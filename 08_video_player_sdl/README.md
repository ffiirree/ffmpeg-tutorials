# Video Player with SDL2

使用SDL库渲染视频，Qt虽然可以方便的绘制图片，但是使用`QOpenGLWidget/OpenGL`加速的代码相对复杂，
而SDL窗口支持使用`OpenGL`直接渲染`YUV420P`格式的视频帧。

## SDL渲染YUV视频帧步骤

1. 首先初始化SDL

```C++
// 仅视频
CHECK(SDL_Init(SDL_INIT_VIDEO) >= 0);
```

2. 创建窗口指定`SDL_WINDOW_OPENGL`

```C++
// 标题和尺寸还可以单独设置
CHECK_NOTNULL(window_ = SDL_CreateWindow("Player",
                                         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                         decoder_->width(), decoder_->height(),
                                         SDL_WINDOW_OPENGL
));
```

3. 创建渲染器

```C++
CHECK_NOTNULL(renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED  | SDL_RENDERER_PRESENTVSYNC));

// 或者
CHECK_NOTNULL(renderer_ = SDL_CreateRenderer(window_, -1, 0));
```

4. 创建渲染YUV的纹理

```C++
// SDL_PIXELFORMAT_IYUV 对应 ffmpeg中的 AV_PIX_FMT_YUV420P
CHECK_NOTNULL(texture_ = SDL_CreateTexture(renderer_,
                                           SDL_PIXELFORMAT_IYUV,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           decoder_->width(),decoder_->height()
));
```

5. 渲染视频帧

```C++
// 将视频帧渲染到窗口的rect区域
SDL_Rect rect{ 0, 0, frame_->width, frame_->height };
SDL_UpdateYUVTexture(texture_, &rect,
                     frame_->data[0], frame_->linesize[0],
                     frame_->data[1], frame_->linesize[1],
                     frame_->data[2], frame_->linesize[2]
);

SDL_RenderClear(renderer_);
SDL_RenderCopy(renderer_, texture_, nullptr, &rect);
SDL_RenderPresent(renderer_);
```

6. 退出时销毁资源

```C++
// 销毁窗口相关
SDL_DestroyWindow(window_);
SDL_DestroyRenderer(renderer_);
SDL_DestroyTexture(texture_);

// SDL退出
SDL_Quit();
```

## 架构设计

本示例共使用两个线程：**主线程**负责SDL的*事件循环*(包括界面刷新) 和 **解码线程** 负责解码视频，并控制播放速度，通过回调函数发送SDL界面刷新的事件。

解码线程 和之前的流程没有区别，这里说一下主线程的SDL事件循环。本次一共使用了2个事件：自定义的`SDL_USEREVENT + 1`作为刷线界面的事件 和 `SDL_QUIT` 退出事件。

```C++
// 循环直到关闭窗口，退出程序
for(;;) {
    SDL_Event event;

    // 等待直到获取到一个事件
    SDL_WaitEvent(&event);

    switch (event.type) {
        // 在解码线程的回调时，发送该事件
        case SDL_USEREVENT + 1:
            // 调用player的渲染视频帧的函数
            player.update();
            break;

        // 关闭窗口时，会产生这个事件
        case SDL_QUIT:
            SDL_Quit(); // 退出
            return 0;

        default: break;
    }
}
```

视频渲染事件在解码线程回调时发送：

```C++
SDL_Event event;
event.type = SDL_USEREVENT + 1;
SDL_PushEvent(&event);
```