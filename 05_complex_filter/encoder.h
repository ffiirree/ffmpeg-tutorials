#ifndef _05_ENCODER_H
#define _05_ENCODER_H

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


class Encoder {
public:
    Encoder()
    {
        packet_ = av_packet_alloc();
    }
    ~Encoder()
    {
        av_write_trailer(fmt_ctx_);

        avformat_free_context(fmt_ctx_);
        if (fmt_ctx_ && !(fmt_ctx_->oformat->flags & AVFMT_NOFILE))
            avio_closep(&fmt_ctx_->pb);

        avcodec_free_context(&video_encode_ctx_);
        avcodec_free_context(&audio_encode_ctx_);

        av_packet_free(&packet_);
    }

    int open(const std::string& filename, int w, int h, AVPixelFormat format, AVRational sar, AVRational framerate, AVRational time_base)
    {
        CHECK(avformat_alloc_output_context2(&fmt_ctx_, nullptr, nullptr, filename.c_str()) >= 0);

        CHECK_NOTNULL(avformat_new_stream(fmt_ctx_, nullptr));
        video_stream_idx_ = 0;

        video_encoder_ = avcodec_find_encoder_by_name("libx265");
        CHECK_NOTNULL(video_encoder_);
        video_encode_ctx_ = avcodec_alloc_context3(video_encoder_);
        CHECK_NOTNULL(video_encode_ctx_);

        AVDictionary* encoder_options = nullptr;
        av_dict_set(&encoder_options, "crf", "23", AV_DICT_DONT_OVERWRITE);
        av_dict_set(&encoder_options, "threads", "auto", AV_DICT_DONT_OVERWRITE);
        defer(av_dict_free(&encoder_options));

        // encoder codec params
        video_encode_ctx_->height = h;
        video_encode_ctx_->width = w;
        video_encode_ctx_->sample_aspect_ratio = sar;
        video_encode_ctx_->pix_fmt = format;

        // time base
        video_encode_ctx_->time_base = time_base;
        fmt_ctx_->streams[video_stream_idx_]->time_base = time_base;
        video_encode_ctx_->framerate = framerate;

        if (fmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER) {
            video_encode_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        CHECK(avcodec_open2(video_encode_ctx_, video_encoder_, &encoder_options) >= 0);
        CHECK(avcodec_parameters_from_context(fmt_ctx_->streams[video_stream_idx_]->codecpar, video_encode_ctx_) >= 0);

        if(!(fmt_ctx_->oformat->flags & AVFMT_NOFILE)) {
            CHECK(avio_open(&fmt_ctx_->pb, filename.c_str(), AVIO_FLAG_WRITE) >= 0);
        }

        CHECK(avformat_write_header(fmt_ctx_, nullptr) >= 0);

        av_dump_format(fmt_ctx_, 0, filename.c_str(), 1);
        return 0;
    }

    int encode_frame(AVFrame * frame)
    {
        frame->pict_type = AV_PICTURE_TYPE_NONE;

        if((!frame->width && !frame->height))
            LOG(INFO) << "[ENCODER] NULL";

        int ret = avcodec_send_frame(video_encode_ctx_, (!frame->width && !frame->height) ? nullptr : frame);
        while(ret >= 0) {
            av_packet_unref(packet_);
            ret = avcodec_receive_packet(video_encode_ctx_, packet_);

            if(ret == AVERROR(EAGAIN)) {
                break;
            }
            else if (ret == AVERROR_EOF) {
                LOG(INFO) << "[ENCODER] EOF";
                break;
            }
            else if(ret < 0) {
                LOG(ERROR) << "avcodec_receive_packet()";
                return -1;
            }

            packet_->stream_index = video_stream_idx_;
            LOG(INFO) << fmt::format("[ENCODER] pts = {}, frame = {}", packet_->pts, video_encode_ctx_->frame_number);
            av_packet_rescale_ts(packet_, video_encode_ctx_->time_base, fmt_ctx_->streams[video_stream_idx_]->time_base);

            if (av_interleaved_write_frame(fmt_ctx_, packet_) != 0) {
                LOG(ERROR) << "av_interleaved_write_frame()";
                return -1;
            }
        }

        return ret;
    }
//private:
    AVFormatContext * fmt_ctx_{nullptr};
    AVCodecContext * video_encode_ctx_{nullptr};
    AVCodecContext * audio_encode_ctx_{nullptr};

    AVCodec * video_encoder_{nullptr};
    AVCodec * audio_encoder_{nullptr};

    int video_stream_idx_{ 0 };
    int audio_stream_idx_{ -1 };

    AVPacket * packet_{nullptr};
};

#endif //!_05_ENCODER_H