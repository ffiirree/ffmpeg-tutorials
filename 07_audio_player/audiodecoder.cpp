#include "audiodecoder.h"
#include "fmt/core.h"

int AudioDecoder::open(const std::string& name)
{
    if (running_ || opened_ || fmt_ctx_) {
        close();
    }

    // format context
    CHECK_NOTNULL(fmt_ctx_ = avformat_alloc_context());
    // open input
    CHECK(avformat_open_input(&fmt_ctx_, name.c_str(), nullptr, nullptr) >= 0);
    CHECK(avformat_find_stream_info(fmt_ctx_, nullptr) >= 0);

    av_dump_format(fmt_ctx_, 0, name.c_str(), 0);

    // find  audio stream
    audio_stream_index_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    CHECK(audio_stream_index_ >= 0);

    //  audio decoder context
    CHECK_NOTNULL(audio_decoder_ = avcodec_find_decoder(fmt_ctx_->streams[audio_stream_index_]->codecpar->codec_id));
    CHECK_NOTNULL(audio_decoder_ctx_ = avcodec_alloc_context3(audio_decoder_));

    CHECK(avcodec_parameters_to_context(audio_decoder_ctx_, fmt_ctx_->streams[audio_stream_index_]->codecpar) >= 0);

    CHECK(avcodec_open2(audio_decoder_ctx_, audio_decoder_, nullptr) >= 0);

    sample_rate_ = audio_decoder_ctx_->sample_rate;
    channels_ = audio_decoder_ctx_->channels;
    channel_layout_ = audio_decoder_ctx_->channel_layout;
    channel_layout_ = (channel_layout_ == 0) ? av_get_default_channel_layout(channels_) : channel_layout_;
    sample_fmt_ = audio_decoder_ctx_->sample_fmt;

    LOG(INFO) << fmt::format("[DECODER] SAMPLE_RATE = {}, CHANNELS = {}, CHANNEL_LAYOUT = {}, SAMPLE_FMT = {}, SAMPLE_SIZE = {}",
                             sample_rate_, channels_,
                             channel_layout_,
                             av_get_sample_fmt_name(sample_fmt_),
                             av_get_bytes_per_sample(sample_fmt_) * 8
    );


    // prepare
    packet_ = av_packet_alloc();
    decoded_frame_ = av_frame_alloc();

    opened_ = true;
    LOG(INFO) << "[DECODER]: " << name << " is opened";
    return 0;
}

void AudioDecoder::audio_thread_f()
{
    LOG(INFO) << "[AUDIO THREAD] STARTED@" << std::this_thread::get_id();
    defer(LOG(INFO) << "[AUDIO THREAD] EXITED");

    while (running_) {
        av_packet_unref(packet_);
        int ret = av_read_frame(fmt_ctx_, packet_);
        if (ret < 0) {
            if ((ret == AVERROR_EOF || avio_feof(fmt_ctx_->pb)) && !(eof_)) {
                LOG(INFO) << "[AUDIO THREAD] PUT NULL PACKET TO FLUSH DECODERS";
                // [flushing] 1. Instead of valid input, send NULL to the avcodec_send_packet() (decoding) or avcodec_send_frame() (encoding) functions. This will enter draining mode.
                // [flushing] 2. Call avcodec_receive_frame() (decoding) or avcodec_receive_packet() (encoding) in a loop until AVERROR_EOF is returned.The functions will not return AVERROR(EAGAIN), unless you forgot to enter draining mode.
                av_packet_unref(packet_);
                packet_->stream_index = audio_stream_index_;
                eof_ = true;
            }
            else {
                LOG(ERROR) << "[AUDIO THREAD] read frame failed";
                break;
            }
        }

        first_pts_ = (first_pts_ == AV_NOPTS_VALUE) ? av_gettime_relative() : first_pts_;

        if (packet_->stream_index == audio_stream_index_) {
            ret = avcodec_send_packet(audio_decoder_ctx_, packet_);
            while (ret >= 0) {
                av_frame_unref(decoded_frame_);
                ret = avcodec_receive_frame(audio_decoder_ctx_, decoded_frame_);
                if (ret == AVERROR(EAGAIN) ) {
                    break;
                }
                else if (ret == AVERROR_EOF) {
                    running_ = false;
                    LOG(INFO) << "[AUDIO THREAD] EOF";
                    break;
                }
                else if (ret < 0) { // error, exit
                    LOG(ERROR) << "[AUDIO THREAD] legitimate decoding errors";
                    running_ = false;
                    break;
                }

                decoded_frame_->pts -= fmt_ctx_->streams[packet_->stream_index]->start_time;
                LOG(INFO) << fmt::format("[AUDIO THREAD] pts = {}", decoded_frame_->pts);

                // decoded frame@{
                while(buffer_.full() && running_) {
                    av_usleep(30'0000);
                }

                buffer_.push([=](AVFrame* pushed){
                    av_frame_unref(pushed);
                    av_frame_move_ref(pushed, decoded_frame_);
                });
                // @}
            }
        }
    }
}

void AudioDecoder::close()
{
    running_ = false;
    opened_ = false;
    eof_ = false;

    if (audio_thread_.joinable()) {
        audio_thread_.join();
    }

    first_pts_ = AV_NOPTS_VALUE;

    av_packet_free(&packet_);
    av_frame_free(&decoded_frame_);

    avcodec_free_context(&audio_decoder_ctx_);
    avformat_close_input(&fmt_ctx_);

    buffer_.clear();

    LOG(INFO) << "[DECODER] CLOSED";
}
