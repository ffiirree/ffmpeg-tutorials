#ifndef _05_DECODER_H
#define _05_DECODER_H

#include <vector>
#include <string>
#include <thread>
#include <atomic>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
}

#include "defer.h"
#include "logging.h"
#include "ringvector.h"
#include "fmt/format.h"

class Decoder {
public:
    Decoder()
    {
        packet_ = av_packet_alloc();
        video_frame_ = av_frame_alloc();
        audio_frame_ = av_frame_alloc();
    }
    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;

    ~Decoder()
    {
        avformat_close_input(&fmt_ctx_);

        avcodec_free_context(&video_decode_ctx_);
        avcodec_free_context(&audio_decode_ctx_);

        av_packet_free(&packet_);
        av_frame_free(&video_frame_);
        av_frame_free(&audio_frame_);
    }

    int open(const std::string& filename)
    {
        LOG(INFO) << filename;
        CHECK(avformat_open_input(&fmt_ctx_, filename.c_str(), nullptr, nullptr) >= 0);
        CHECK(avformat_find_stream_info(fmt_ctx_, nullptr) >= 0);

        video_stream_idx_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        audio_stream_idx_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        CHECK(video_stream_idx_ >= 0 || audio_stream_idx_ >= 0);

        // decoder
        LOG(INFO) << fmt_ctx_->streams[video_stream_idx_]->codecpar->codec_id;
        auto video_decoder = avcodec_find_decoder(fmt_ctx_->streams[video_stream_idx_]->codecpar->codec_id);
        CHECK_NOTNULL(video_decoder);

        // decoder context
        video_decode_ctx_ = avcodec_alloc_context3(video_decoder);
        CHECK_NOTNULL(video_decode_ctx_);

        CHECK(avcodec_parameters_to_context(video_decode_ctx_, fmt_ctx_->streams[video_stream_idx_]->codecpar) >= 0);

        AVDictionary * decoder_options = nullptr;
        av_dict_set(&decoder_options, "threads", "auto", AV_DICT_DONT_OVERWRITE);
        CHECK(avcodec_open2(video_decode_ctx_, video_decoder, &decoder_options) >= 0);

        av_dump_format(fmt_ctx_, 0, filename.c_str(), 0);
        LOG(INFO) << fmt::format("[ INPUT] {}: {}x{}, fps = {}/{}, tbr = {}/{}, tbc = {}/{}, tbn = {}/{}\n",
                                 filename,
                                 video_decode_ctx_->width, video_decode_ctx_->height,
                                 fmt_ctx_->streams[video_stream_idx_]->avg_frame_rate.num, fmt_ctx_->streams[video_stream_idx_]->avg_frame_rate.den,
                                 fmt_ctx_->streams[video_stream_idx_]->r_frame_rate.num, fmt_ctx_->streams[video_stream_idx_]->r_frame_rate.den,
                                 video_decode_ctx_->time_base.num, video_decode_ctx_->time_base.den,
                                 fmt_ctx_->streams[video_stream_idx_]->time_base.num, fmt_ctx_->streams[video_stream_idx_]->time_base.den);

        return 0;
    }

    void decode_thread()
    {
        LOG(INFO) << "[DECODER THREAD @ " << std::this_thread::get_id() << "] START";
        defer(LOG(INFO) << "[DECODER THREAD @ " << std::this_thread::get_id() << "] EXITED");

        if (video_stream_idx_ < 0) eof_ |= 0x01;
        if (audio_stream_idx_ < 0) eof_ |= 0x02;

        while(running_ && !(eof_ & 0b0100)) {
            if (video_frame_buffer_.full() || audio_frame_buffer_.full()) {
                av_usleep(20000);
                continue;
            }

            av_packet_unref(packet_);
            int ret = av_read_frame(fmt_ctx_, packet_);
            if (ret < 0) {
                if ((ret == AVERROR_EOF || avio_feof(fmt_ctx_->pb)) && !(eof_ & 0b0100)) {
                    LOG(INFO) << "[DECODER THREAD] PUT NULL PACKET TO FLUSH DECODERS";
                    av_packet_unref(packet_);
                    eof_ |= 0b0100;
                }
                else {
                    LOG(ERROR) << "[DECODER THREAD] read frame failed";
                    break;
                }
            }

            // video packet
            if (packet_->stream_index == video_stream_idx_ || (eof_ & 0b0100)) {
                ret = avcodec_send_packet(video_decode_ctx_, packet_);
                while (ret >= 0) {
                    av_frame_unref(video_frame_);
                    ret = avcodec_receive_frame(video_decode_ctx_, video_frame_);
                    if (ret == AVERROR(EAGAIN)) {
                        break;
                    }
                    else if (ret == AVERROR_EOF) { // fully flushed, exit
                        LOG(INFO) << "[DECODER THREAD @ " << std::this_thread::get_id() << "] VIDEO EOF";
                        eof_ |= 0x01;
                        break;
                    }
                    else if (ret < 0) { // error, exit
                        LOG(ERROR) << "[DECODER THREAD @ " << std::this_thread::get_id() << "] legitimate decoding errors";
                        running_ = false;
                        break;
                    }

                    LOG(INFO) << "[DECODER THREAD @ " << std::this_thread::get_id() << "] pts = " << video_frame_->pts
                              << ", frame = " << video_decode_ctx_->frame_number;

                    while (video_frame_buffer_.full() && running_) {
                        av_usleep(20000);
                    }
                    video_frame_buffer_.push([this](AVFrame *frame) {
                        av_frame_unref(frame);
                        av_frame_move_ref(frame, video_frame_);
                    });
                }
            }

            // audio packet
            if(packet_->stream_index == audio_stream_idx_ || (eof_ & 0b0100)) {
                // TODO:
            }
        }

        if (video_stream_idx_ >= 0) video_frame_buffer_.push([](AVFrame *nil) { av_frame_unref(nil); });
        if (audio_stream_idx_ >= 0) audio_frame_buffer_.push([](AVFrame *nil) { av_frame_unref(nil); });

        running_ = false;
        eof_ = 0b0111;
    }

    std::string filter_args()
    {
        auto video_stream = fmt_ctx_->streams[video_stream_idx_];
        return fmt::format(
                "video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}",
                video_decode_ctx_->width, video_decode_ctx_->height, video_decode_ctx_->pix_fmt,
                video_stream->time_base.num, video_stream->time_base.den,
                video_stream->sample_aspect_ratio.num, video_stream->sample_aspect_ratio.den
        );
    }

    bool eof() const { return eof_ == 0b0111; }

//private:
    std::atomic<bool> running_{false};
    std::atomic<uint8_t> eof_{ 0x00 };

    int video_stream_idx_{-1};
    int audio_stream_idx_{-1};
    AVFormatContext * fmt_ctx_{nullptr};

    AVCodecContext * video_decode_ctx_{nullptr};
    AVCodecContext * audio_decode_ctx_{nullptr};

    AVPacket *packet_{nullptr};
    AVFrame * video_frame_{nullptr};
    AVFrame * audio_frame_{nullptr};

    RingVector<AVFrame*, 3> video_frame_buffer_{
            []() { return av_frame_alloc(); },
            [](AVFrame** frame) { av_frame_free(frame); }
    };

    RingVector<AVFrame*, 9> audio_frame_buffer_{
            []() { return av_frame_alloc(); },
            [](AVFrame** frame) { av_frame_free(frame); }
    };
};

#endif //!_05_DECODER_H