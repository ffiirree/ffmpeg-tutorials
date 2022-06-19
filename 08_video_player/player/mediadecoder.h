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
}
#include <atomic>
#include <mutex>
#include <thread>
#include <map>
#include <functional>
#include "defer.h"
#include "logging.h"

class MediaDecoder {
public:
    MediaDecoder() = default;
    MediaDecoder(const MediaDecoder&) = delete;
    MediaDecoder& operator=(const MediaDecoder&) = delete;
    ~MediaDecoder() { close(); }

    bool open(const std::string& name, const std::string& format, const std::string& filters_descr, AVPixelFormat pix_fmt,
              const std::map<std::string, std::string>& options);
    bool create_filters();

    bool opened() { return opened_; }
    bool running() { return running_; }

    void start();
    void video_thread_f();

    int width() { return video_decoder_ctx_ ? video_decoder_ctx_->width : 480; }
    int height() { return video_decoder_ctx_ ? video_decoder_ctx_->height : 360; }

    void set_video_callback(std::function<void(AVFrame *)> callback) { video_callback_ = std::move(callback); }

private:
    void close();

    std::atomic<bool> running_{ false };
    std::atomic<bool> opened_{ false };

    std::thread video_thread_;

    AVFormatContext* fmt_ctx_{ nullptr };
    int video_stream_index_{ -1 };

    AVCodecContext* video_decoder_ctx_{ nullptr };
    AVCodec* video_decoder_{ nullptr };

    AVPixelFormat pix_fmt_{ AV_PIX_FMT_YUV420P };

    AVPacket* packet_{ nullptr };

    AVFrame* decoded_video_frame_{ nullptr };
    AVFrame* filtered_frame_{ nullptr };

    int64_t first_pts_{ AV_NOPTS_VALUE };

    std::function<void(AVFrame *)> video_callback_{ [](AVFrame *){ } };

    std::string filters_descr_;
    AVFilterGraph* filter_graph_{ nullptr };
    AVFilterContext* buffersrc_ctx_{ nullptr };
    AVFilterContext* buffersink_ctx_{ nullptr };
};

#endif // !PLAYER_MEDIA_DECODER
