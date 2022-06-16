#ifndef PLAYER_AUDIO_DECODER
#define PLAYER_AUDIO_DECODER

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
}
#include <atomic>
#include <mutex>
#include <thread>
#include "ringvector.h"
#include "utils.h"
#include "logging.h"

class AudioDecoder {
public:
    AudioDecoder() = default;
    AudioDecoder(const AudioDecoder&) = delete;
    AudioDecoder& operator=(const AudioDecoder&) = delete;
    ~AudioDecoder() { destroy(); }

    int open(const std::string& name);

    bool running() { return running_; }

    void run()
    {
        if (!opened_ || running_) {
            LOG(ERROR) << "[DECODER] already running or not opened";
            return;
        }

        running_ = true;
        audio_thread_ = std::thread([this](){ audio_thread_f(); });
    }

    void audio_thread_f();

    bool empty() const { return buffer_.empty(); }

    int produce(AVFrame * frame)
    {
        if (empty()) return AVERROR(EAGAIN);
        if (empty() && eof_) return AVERROR_EOF;

        buffer_.pop([frame](AVFrame* popped){
            av_frame_unref(frame);
            av_frame_move_ref(frame, popped);
        });

        return 0;
    }

    int sample_rate() const { return audio_decoder_ctx_ ? audio_decoder_ctx_->sample_rate : 44100; }
    enum AVSampleFormat format() const { return audio_decoder_ctx_ ? audio_decoder_ctx_->sample_fmt : AV_SAMPLE_FMT_NONE; }
    int channels() const { return audio_decoder_ctx_ ? audio_decoder_ctx_->channels : 0; }
    uint64_t channel_layout() const
    {
        return audio_decoder_ctx_ && audio_decoder_ctx_->channel_layout ?
               audio_decoder_ctx_->channel_layout :
               av_get_default_channel_layout(channels());
    }

private:
    void destroy();

    std::atomic<bool> running_{ false };
    std::atomic<bool> opened_{ false };
    std::atomic<bool> eof_{ false };

    std::thread audio_thread_;

    AVFormatContext* fmt_ctx_{ nullptr };
    int audio_stream_index_{ -1 };

    AVCodecContext* audio_decoder_ctx_{ nullptr };
    AVCodec* audio_decoder_{ nullptr };

    AVPacket* packet_{ nullptr };
    AVFrame* decoded_frame_{ nullptr };

    RingVector<AVFrame*, 16> buffer_{
            []() { return av_frame_alloc(); },
            [](AVFrame** frame) { av_frame_free(frame); }
    };
};

#endif // !PLAYER_AUDIO_DECODER
