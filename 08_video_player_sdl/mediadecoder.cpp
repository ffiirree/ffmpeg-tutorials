#include "mediadecoder.h"
#include "fmt/core.h"
#include "fmt/ranges.h"

bool MediaDecoder::open(const std::string& name,
                        const std::string& format,
                        const std::map<std::string, std::string>& options)
{
    LOG(INFO) << fmt::format("[DECODER] options = {}", options);

    if (running_ || opened_ || fmt_ctx_) {
        close();
    }

    avdevice_register_all();

    // format context
    CHECK_NOTNULL(fmt_ctx_ = avformat_alloc_context());

    // input format
    const AVInputFormat* input_fmt = nullptr;
    if (!format.empty()) {
        CHECK_NOTNULL(input_fmt = av_find_input_format(format.c_str()));
    }

    // options
    AVDictionary* input_options = nullptr;
    defer(av_dict_free(&input_options));
    for (const auto& [key, value] : options) {
        av_dict_set(&input_options, key.c_str(), value.c_str(), 0);
    }

    // open input
    CHECK(avformat_open_input(&fmt_ctx_, name.c_str(), input_fmt, &input_options) >= 0);
    CHECK(avformat_find_stream_info(fmt_ctx_, nullptr) >= 0);

    av_dump_format(fmt_ctx_, 0, name.c_str(), 0);

    // find video & audio stream
    video_stream_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    CHECK(video_stream_index_ >= 0);

    // decoder
    const AVCodec* video_decoder = nullptr;
    CHECK_NOTNULL(video_decoder = avcodec_find_decoder(fmt_ctx_->streams[video_stream_index_]->codecpar->codec_id));
    // video decoder context
    CHECK_NOTNULL(video_decoder_ctx_ = avcodec_alloc_context3(video_decoder));
    CHECK(avcodec_parameters_to_context(video_decoder_ctx_, fmt_ctx_->streams[video_stream_index_]->codecpar) >= 0);

    // open codec
    AVDictionary* decoder_options = nullptr;
    defer(av_dict_free(&decoder_options));
    av_dict_set(&decoder_options, "threads", "auto", 0);
    CHECK(avcodec_open2(video_decoder_ctx_, video_decoder, &decoder_options) >= 0);


    LOG(INFO) << fmt::format("[DECODER] FRAMERATE = {}, CFR = {}, STREAM_TIMEBASE = {}/{}",
                             video_decoder_ctx_->framerate.num, video_decoder_ctx_->framerate.num == video_decoder_ctx_->time_base.den,
                             video_decoder_ctx_->time_base.num, video_decoder_ctx_->time_base.den
    );

    // prepare
    packet_ = av_packet_alloc();
    decoded_video_frame_ = av_frame_alloc();

    opened_ = true;
    LOG(INFO) << "[DECODER]: " << name << " is opened";
    return true;
}

void MediaDecoder::start()
{
    if (!opened_ || running_) {
        LOG(ERROR) << "[DECODER] already running or not opened";
        return;
    }

    running_ = true;
    video_thread_ = std::thread([this](){ this->video_thread_f(); });
}

void MediaDecoder::video_thread_f()
{
    LOG(INFO) << "[VIDEO THREAD] STARTED@" << std::this_thread::get_id();
    defer(LOG(INFO) << "[VIDEO THREAD] EXITED");

    while (running()) {
        av_packet_unref(packet_);
        int ret = av_read_frame(fmt_ctx_, packet_);
        if (ret < 0) {
            if ((ret == AVERROR_EOF || avio_feof(fmt_ctx_->pb))) {
                LOG(INFO) << "[VIDEO THREAD] PUT NULL PACKET TO FLUSH DECODERS";
                // [flushing] 1. Instead of valid input, send NULL to the avcodec_send_packet() (decoding) or avcodec_send_frame() (encoding) functions. This will enter draining mode.
                // [flushing] 2. Call avcodec_receive_frame() (decoding) or avcodec_receive_packet() (encoding) in a loop until AVERROR_EOF is returned.The functions will not return AVERROR(EAGAIN), unless you forgot to enter draining mode.
                av_packet_unref(packet_);
                packet_->stream_index = video_stream_index_;
            }
            else {
                LOG(ERROR) << "[VIDEO THREAD] read frame failed";
                break;
            }
        }

        if (packet_->stream_index == video_stream_index_) {
            ret = avcodec_send_packet(video_decoder_ctx_, packet_);
            while (ret >= 0) {
                av_frame_unref(decoded_video_frame_);
                ret = avcodec_receive_frame(video_decoder_ctx_, decoded_video_frame_);
                if (ret == AVERROR(EAGAIN)) {
                    break;
                }
                else if (ret == AVERROR_EOF) { // fully flushed, exit
                    // [flushing] 3. Before decoding can be resumed again, the codec has to be reset with avcodec_flush_buffers()
                    avcodec_flush_buffers(video_decoder_ctx_);
                    LOG(INFO) << "[VIDEO THREAD] EOF";
                    return;
                }
                else if (ret < 0) { // error, exit
                    LOG(ERROR) << "[VIDEO THREAD] legitimate decoding errors";
                    return;
                }

                first_pts_ = (first_pts_ == AV_NOPTS_VALUE) ? av_gettime_relative() : first_pts_;
                decoded_video_frame_->pts = (decoded_video_frame_->pts == AV_NOPTS_VALUE) ?
                                            av_rescale_q(av_gettime_relative() - first_pts_, { 1, AV_TIME_BASE }, fmt_ctx_->streams[packet_->stream_index]->time_base) :
                                            decoded_video_frame_->pts - fmt_ctx_->streams[packet_->stream_index]->start_time;

                int64_t ts = av_gettime_relative() - first_pts_;

                int64_t pts_us = av_rescale_q(std::max<int64_t>(0, decoded_video_frame_->pts), fmt_ctx_->streams[packet_->stream_index]->time_base, { 1, AV_TIME_BASE });
                int64_t duration_us = av_rescale_q(decoded_video_frame_->pkt_duration, fmt_ctx_->streams[packet_->stream_index]->time_base, { 1, AV_TIME_BASE });
                int64_t sleep_us = decoded_video_frame_->pkt_duration > 0 ? duration_us : pts_us - ts;

                LOG(INFO) << fmt::format("[VIDEO THREAD] pts = {:>6.3f}s, ts = {:>6.3f}s, sleep = {:>4d}ms, frame = {:>4d}, fps = {:>5.2f}",
                                         pts_us / 1000000.0, ts / 1000000.0, sleep_us / 1000,
                                         video_decoder_ctx_->frame_number, video_decoder_ctx_->frame_number / (ts / 1000000.0));

                av_usleep(sleep_us);

                video_callback_(decoded_video_frame_);
            }
        }
    }
}

void MediaDecoder::close()
{
    running_ = false;
    opened_ = false;

    wait();

    first_pts_ = AV_NOPTS_VALUE;

    av_packet_free(&packet_);

    av_frame_free(&decoded_video_frame_);
    avcodec_free_context(&video_decoder_ctx_);
    avformat_close_input(&fmt_ctx_);

    LOG(INFO) << "[DECODER] CLOSED";
}
