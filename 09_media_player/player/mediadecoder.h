#ifndef PLAYER_MEDIA_DECODER
#define PLAYER_MEDIA_DECODER

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavutil/time.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libswresample/swresample.h>
}
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "ringvector.h"
#include "ringbuffer.h"
#include "utils.h"
#include "logging.h"

const int BUFFER_SIZE = 20;

class MediaDecoder  {
public:
    MediaDecoder() = default;
    MediaDecoder(const MediaDecoder&) = delete;
    MediaDecoder& operator=(const MediaDecoder&) = delete;
    ~MediaDecoder() { close(); }

    bool open(const string& name, const string& format, const string& filters_descr, AVPixelFormat pix_fmt, const map<string, string>& options);
    bool create_filters();

    bool opened() { return opened_; }
    bool running() { return running_; }
    bool paused() { return paused_; }

    void start()
    {
        if (!opened() || running_) {
            LOG(ERROR) << "[DECODER] already running or not opened";
            return;
        }

        running_ = true;

        read_thread_ = std::thread([this](){ this->read_thread_f(); });
        video_thread_ = std::thread([this](){ this->video_thread_f(); });
        audio_thread_ = std::thread([this](){ this->audio_thread_f(); });
    }

    void read_thread_f();
    void video_thread_f();
    void audio_thread_f();

    int width() { return video_decoder_ctx_ ? video_decoder_ctx_->width : 480; }
    int height() { return video_decoder_ctx_ ? video_decoder_ctx_->height : 360; }
    AVRational sar() { return video_decoder_ctx_ ? video_decoder_ctx_->sample_aspect_ratio : AVRational{ 0, 1 }; }
    AVRational framerate() { return opened() ? av_guess_frame_rate(fmt_ctx_, fmt_ctx_->streams[video_stream_index_], nullptr) : AVRational{ 30, 1 }; }
    AVRational timebase() { return opened() ? fmt_ctx_->streams[video_stream_index_]->time_base : AVRational{ 1, AV_TIME_BASE }; }

    void set_video_callback(std::function<void(AVFrame *)> callback) { video_callback_ = std::move(callback); }
    void set_audio_callback(std::function<std::pair<int64_t, bool>(RingBuffer&)> callback) { audio_callback_ = std::move(callback); }
    void set_period_size(size_t size) { period_size_ = size; }

    void pause() { paused_ = true; }
    void resume() { paused_ = false; }

private:
    void close();

    std::atomic<bool> running_{ false };
    std::atomic<bool> paused_{ false };
    std::atomic<bool> opened_{ false };

    std::thread read_thread_;
    std::thread video_thread_;
    std::thread audio_thread_;

    AVFormatContext* fmt_ctx_{ nullptr };
    int video_stream_index_{ -1 };
    int audio_stream_index_{ -1 };

    AVCodecContext* video_decoder_ctx_{ nullptr };
    AVCodecContext* audio_decoder_ctx_{ nullptr };
    AVCodec* video_decoder_{ nullptr };
    AVCodec* audio_decoder_{ nullptr };

    AVPixelFormat pix_fmt_{ AV_PIX_FMT_YUV420P };

    AVPacket* packet_{ nullptr };
    AVPacket* video_packet_{ nullptr };
    AVPacket* audio_packet_{ nullptr };

    AVFrame* decoded_video_frame_{ nullptr };
    AVFrame* decoded_audio_frame_{ nullptr };
    AVFrame* filtered_frame_{ nullptr };

    SwrContext * swr_ctx_{ nullptr };

    int64_t first_pts_{ AV_NOPTS_VALUE };
    std::atomic<int64_t> audio_clock_{ 0 }; // { 1, AV_TIME_BASE } unit
    std::atomic<int64_t> audio_clock_ts_{ 0 };

    int64_t clock_us()
    {
        if (audio_stream_index_ < 0) {
            return av_gettime_relative() - first_pts_;
        }
        return std::max<int64_t>(0, audio_clock_ + av_gettime_relative() - audio_clock_ts_);
    }

    double clock_s()
    {
        return (double) clock_us() / (double) AV_TIME_BASE;
    }

    RingVector<AVPacket*, BUFFER_SIZE> video_packet_buffer_{
            []() { return av_packet_alloc(); },
            [](AVPacket** packet) { av_packet_free(packet); }
    };

    RingVector<AVPacket*, BUFFER_SIZE> audio_packet_buffer_{
            []() { return av_packet_alloc(); },
            [](AVPacket** packet) { av_packet_free(packet); }
    };

    size_t period_size_{ 4096 * 2 };

    std::function<void(AVFrame *)> video_callback_{ [](AVFrame *){ } };
    std::function<std::pair<int64_t, bool>(RingBuffer&)> audio_callback_{ [](RingBuffer&) { return std::pair{0, false}; } };

    string filters_descr_;
    AVFilterGraph* filter_graph_{ nullptr };
    AVFilterContext* buffersrc_ctx_{ nullptr };
    AVFilterContext* buffersink_ctx_{ nullptr };
};

#endif // !PLAYER_MEDIA_DECODER
