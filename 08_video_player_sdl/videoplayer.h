#ifndef PLAYER_VIDEO_PLAYER_H
#define PLAYER_VIDEO_PLAYER_H

extern "C" {
#include "SDL.h"
};
#include "mediadecoder.h"
#include "logging.h"
#include "fmt/format.h"

class VideoPlayer {

public:
    explicit VideoPlayer();
    ~VideoPlayer();

    bool play(const std::string& name, const std::string& fmt = "");

    void update();

private:
    SDL_Window * window_{ nullptr };
    SDL_Renderer* renderer_{ nullptr };
    SDL_Texture* texture_{ nullptr };

    std::unique_ptr<MediaDecoder> decoder_{ nullptr };

    AVFrame *frame_{ nullptr };
    std::mutex mtx_;
};

#endif // !PLAYER_VIDEO_PLAYER_H