#include "videoplayer.h"

VideoPlayer::VideoPlayer()
{
    frame_ = av_frame_alloc();
    decoder_ = std::make_unique<MediaDecoder>();

    decoder_->set_video_callback([=](AVFrame * frame) {
        std::lock_guard lock(mtx_);
        if(frame_) {
            av_frame_unref(frame_);
            av_frame_ref(frame_, frame);
        }

        SDL_Event event;
        event.type = SDL_USEREVENT + 1;
        SDL_PushEvent(&event);
    });

    // SDL window
    CHECK_NOTNULL(window_ = SDL_CreateWindow("Player",
                                             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                             decoder_->width(), decoder_->height(),
                                             SDL_WINDOW_OPENGL
    ));

    CHECK_NOTNULL(renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED  | SDL_RENDERER_PRESENTVSYNC));
}

VideoPlayer::~VideoPlayer()
{
    av_frame_free(&frame_);

    SDL_DestroyWindow(window_);
    window_ = nullptr;
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
    SDL_DestroyTexture(texture_);
    texture_ = nullptr;
}

bool VideoPlayer::play(const std::string& name, const std::string& fmt)
{
    if (!decoder_->open(name, fmt, { })) {
        return false;
    }

    decoder_->start();

    SDL_SetWindowTitle(window_, name.c_str());
    SDL_SetWindowSize(window_, decoder_->width(), decoder_->height());

    CHECK_NOTNULL(texture_ = SDL_CreateTexture(renderer_,
                                               SDL_PIXELFORMAT_IYUV,
                                               SDL_TEXTUREACCESS_STREAMING,
                                               decoder_->width(), decoder_->height()
    ));

    return true;
}

void VideoPlayer::update()
{
    std::lock_guard lock(mtx_);

    if (frame_ && frame_->format == AV_PIX_FMT_YUV420P) {
        SDL_Rect rect{ 0, 0, frame_->width, frame_->height };
        SDL_UpdateYUVTexture(texture_, &rect,
                             frame_->data[0], frame_->linesize[0],
                             frame_->data[1], frame_->linesize[1],
                             frame_->data[2], frame_->linesize[2]
        );

        SDL_RenderClear(renderer_);
        SDL_RenderCopy(renderer_, texture_, nullptr, &rect);
        SDL_RenderPresent(renderer_);
    }
}