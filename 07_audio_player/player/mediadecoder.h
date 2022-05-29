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
#include <functional>
#include "ringbuffer.h"
#include "utils.h"
#include "logging.h"

const int BUFFER_SIZE = 10;

class MediaDecoder : public QObject {
	Q_OBJECT

public:
	explicit MediaDecoder(QObject *parent = nullptr): QObject(parent) { }
	~MediaDecoder() override {close(); }

	bool open(const string& name);
	bool create_filters();

    bool opened() { return opened_; }
	bool running() { return running_; }

    void start()
    {
        if (!opened() || running_) {
            LOG(ERROR) << "[DECODER] already running or not opened";
            return;
        }

        running_ = true;
        audio_thread_ = std::thread([this](){ this->audio_thread_f(); });
    }

    void audio_thread_f();

    void set_audio_callback(std::function<std::pair<int64_t, bool>(RingBuffer&)> callback) { audio_callback_ = std::move(callback); }
    void set_period_size(int64_t size) { period_size_ = size; }

private:
	void close();

	std::atomic<bool> running_{ false };
    std::atomic<bool> opened_{ false };

    std::thread audio_thread_;

	AVFormatContext* fmt_ctx_{ nullptr };
    int audio_stream_index_{ -1 };

    AVCodecContext* audio_decoder_ctx_{ nullptr };
    AVCodec* audio_decoder_{ nullptr };

    AVPacket* packet_{ nullptr };

    AVFrame* decoded_audio_frame_{ nullptr };

    SwrContext * swr_ctx_{ nullptr };

	int64_t first_pts_{ AV_NOPTS_VALUE };

    int64_t period_size_{ 4096 * 2 };

    std::function<std::pair<int64_t, bool>(RingBuffer&)> audio_callback_{ [](RingBuffer&) { return std::pair{0, false}; } };
};

#endif // !PLAYER_MEDIA_DECODER
