extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}

#include "defer.h"
#include "logging.h"
#include "fmt/format.h"

int main(int argc, char* argv[])
{
    Logger::init(argv[0]);

    if (argc < 3) {
        LOG(ERROR) << "pushing <input_video> <rtmp_name>";
        return -1;
    }

    const char * in_filename = argv[1];
    const char * rtmp_name = argv[2];

    AVFormatContext * decoder_fmt_ctx = nullptr;
    CHECK(avformat_open_input(&decoder_fmt_ctx, in_filename, nullptr, nullptr) >= 0);
    CHECK(avformat_find_stream_info(decoder_fmt_ctx, nullptr) >= 0);

    int video_stream_idx = av_find_best_stream(decoder_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    CHECK(video_stream_idx >= 0);

    // decoder
    auto decoder = avcodec_find_decoder(decoder_fmt_ctx->streams[video_stream_idx]->codecpar->codec_id);
    CHECK_NOTNULL(decoder);

    // decoder context
    AVCodecContext * decoder_ctx = avcodec_alloc_context3(decoder);
    CHECK_NOTNULL(decoder_ctx);

    CHECK(avcodec_parameters_to_context(decoder_ctx, decoder_fmt_ctx->streams[video_stream_idx]->codecpar) >= 0);
    CHECK(avcodec_open2(decoder_ctx, decoder, nullptr) >= 0);

    av_dump_format(decoder_fmt_ctx, 0, in_filename, 0);

    //
    // output
    //
    AVFormatContext * encoder_fmt_ctx = nullptr;
    CHECK(avformat_alloc_output_context2(&encoder_fmt_ctx, nullptr, "flv", nullptr) >= 0);
    CHECK_NOTNULL(avformat_new_stream(encoder_fmt_ctx, nullptr));

    // encoder
    auto encoder = avcodec_find_encoder_by_name("libx264");
    CHECK_NOTNULL(encoder);

    AVCodecContext *encoder_ctx = avcodec_alloc_context3(encoder);
    CHECK_NOTNULL(encoder_ctx);

    AVDictionary* encoder_options = nullptr;
    av_dict_set(&encoder_options, "crf", "23", AV_DICT_DONT_OVERWRITE);
    av_dict_set(&encoder_options, "threads", "auto", AV_DICT_DONT_OVERWRITE);
    defer(av_dict_free(&encoder_options));

    // encoder codec params
    encoder_ctx->height = decoder_ctx->height;
    encoder_ctx->width = decoder_ctx->width;
    encoder_ctx->pix_fmt = decoder_ctx->pix_fmt;
    encoder_ctx->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;
    encoder_ctx->framerate = av_guess_frame_rate(decoder_fmt_ctx, decoder_fmt_ctx->streams[video_stream_idx], nullptr);

    // time base
    encoder_ctx->time_base = av_inv_q(encoder_ctx->framerate);
    encoder_fmt_ctx->streams[0]->time_base = encoder_ctx->time_base;

    CHECK(avcodec_open2(encoder_ctx, encoder, &encoder_options) >= 0);
    CHECK(avcodec_parameters_from_context(encoder_fmt_ctx->streams[0]->codecpar, encoder_ctx) >= 0);

    if (!(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        CHECK(avio_open(&encoder_fmt_ctx->pb, rtmp_name, AVIO_FLAG_WRITE) >= 0);
    }

    CHECK(avformat_write_header(encoder_fmt_ctx, nullptr) >= 0);

    av_dump_format(encoder_fmt_ctx, 0, rtmp_name, 1);

    AVPacket * in_packet = av_packet_alloc();
    defer(av_packet_free(&in_packet));
    AVPacket * out_packet = av_packet_alloc();
    defer(av_packet_free(&out_packet));
    AVFrame * in_frame = av_frame_alloc();
    defer(av_frame_free(&in_frame));

    int64_t first_pts = AV_NOPTS_VALUE;

    while(av_read_frame(decoder_fmt_ctx, in_packet) >= 0) {
        if (in_packet->stream_index != video_stream_idx) {
            continue;
        }

        int ret = avcodec_send_packet(decoder_ctx, in_packet);
        while(ret >= 0) {
            av_frame_unref(in_frame);
            ret = avcodec_receive_frame(decoder_ctx, in_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                LOG(ERROR) << "[PUSHING] avcodec_receive_frame()";
                return ret;
            }

            in_frame->pict_type = AV_PICTURE_TYPE_NONE;

            // sleep @{
            first_pts = first_pts == AV_NOPTS_VALUE ? av_gettime_relative() : first_pts;

            int64_t ts = av_gettime_relative() - first_pts;

            int64_t pts_us = av_rescale_q(in_frame->pts, encoder_fmt_ctx->streams[0]->time_base, { 1, AV_TIME_BASE });
            int64_t sleep_us = std::max<int64_t>(0, pts_us - ts);

            LOG(INFO) << fmt::format("[PUSHING] pts = {:>6.3f}s, ts = {:>6.3f}s, sleep = {:>4d}ms, frame = {:>5d}, fps = {:>5.2f}",
                                     pts_us / 1000000.0, ts / 1000000.0, sleep_us / 1000,
                                     encoder_ctx->frame_number, encoder_ctx->frame_number / (ts / 1000000.0));
            av_usleep(sleep_us);
            // @}

            ret = avcodec_send_frame(encoder_ctx, in_frame);
            while(ret >= 0) {
                av_packet_unref(out_packet);
                ret = avcodec_receive_packet(encoder_ctx, out_packet);

                if(ret == AVERROR(EAGAIN)) {
                    break;
                } else if(ret == AVERROR_EOF) {
                    LOG(INFO) << "[PUSHING] EOF";
                    break;
                } else if(ret < 0) {
                    LOG(ERROR) << "[PUSHING] avcodec_receive_packet()";
                    return ret;
                }

                out_packet->stream_index = 0;
                av_packet_rescale_ts(out_packet, decoder_fmt_ctx->streams[video_stream_idx]->time_base, encoder_fmt_ctx->streams[0]->time_base);

                if (av_interleaved_write_frame(encoder_fmt_ctx, out_packet) != 0) {
                    LOG(ERROR) <<"[PUSHING] av_interleaved_write_frame()";
                    return -1;
                }
            }
        }
        av_packet_unref(in_packet);
    }

    avformat_close_input(&decoder_fmt_ctx);
    avformat_free_context(encoder_fmt_ctx);

    avcodec_free_context(&decoder_ctx);
    avcodec_free_context(&encoder_ctx);

    return 0;
}