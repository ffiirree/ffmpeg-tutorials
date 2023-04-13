extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}
#include "logging.h"
#include "fmt/format.h"
#include "defer.h"
#include "argsparser.h"

enum AVPixelFormat get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *pix_fmt;

    for (pix_fmt = pix_fmts; *pix_fmt != AV_PIX_FMT_NONE; pix_fmt++) {
        const auto desc = av_pix_fmt_desc_get(*pix_fmt);

        if (!(desc->flags & AV_PIX_FMT_FLAG_HWACCEL))
            break;

        const AVCodecHWConfig  *config = nullptr;
        for (auto i = 0; ; ++i) {
            config = avcodec_get_hw_config(ctx->codec, i);
            if (!config) break;

            if (!(config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX))
                continue;

            if (config->pix_fmt == *pix_fmt)
                break;
        }
        break;
    }

    LOG(ERROR) << "pixel format : " << av_get_pix_fmt_name(*pix_fmt);
    return *pix_fmt;
}

int main(int argc, char* argv[])
{
    Logger::init(argv);

    args::parser parser("hw_transcoding -i <input> -hwaccel <hardware> -o <output>", false);
    parser.add("-i", "../h264.mkv", "the input file");
    parser.add("-o", "../hw.mkv", "the output file");
    parser.add("--hwaccel", "cuda", "the hwaccel type");
    parser.add("--decoder", "h264_cuvid", "the encoder");
    parser.add("--encoder", "h264_nvenc", "the decoder");
    parser.add("--v", std::map<std::string, int>{{"x", 1}, {"u", 2}}, "the decoder");
    if (!parser.parse(argc, argv)) {
        LOG(INFO) << parser.help();
    }

    const std::string in_filename = parser.get<std::string>("i").value();
    const std::string out_filename = parser.get<std::string>("o").value();

    AVBufferRef * device_ref = nullptr;
    CHECK(av_hwdevice_ctx_create(&device_ref, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0) >= 0);
    defer(av_buffer_unref(&device_ref));

    // input
    AVFormatContext * decoder_fmt_ctx = avformat_alloc_context();
    CHECK_NOTNULL(decoder_fmt_ctx);

    decoder_fmt_ctx->video_codec = avcodec_find_decoder_by_name(parser.get<std::string>("decoder", "h264_cuvid").c_str());

    CHECK(avformat_open_input(&decoder_fmt_ctx, in_filename.c_str(), nullptr, nullptr) >= 0);
    CHECK(avformat_find_stream_info(decoder_fmt_ctx, nullptr) >= 0);

    int video_stream_idx = av_find_best_stream(decoder_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    CHECK(video_stream_idx >= 0);

    // decoder context
    AVCodecContext * decoder_ctx = nullptr;
    CHECK((decoder_ctx = avcodec_alloc_context3(decoder_fmt_ctx->video_codec)));
    CHECK(avcodec_parameters_to_context(decoder_ctx, decoder_fmt_ctx->streams[video_stream_idx]->codecpar) >= 0);

    // hardware
    decoder_ctx->hw_device_ctx = av_buffer_ref(device_ref);
    CHECK(decoder_ctx->hw_device_ctx);
    decoder_ctx->get_format = get_hw_format;

    CHECK(avcodec_open2(decoder_ctx, decoder_fmt_ctx->video_codec, nullptr) >= 0);

    av_dump_format(decoder_fmt_ctx, 0, in_filename.c_str(), 0);
    LOG(INFO) << fmt::format("[ INPUT] {}x{}, decoder: {}, pix_fmt: {}, fps: {}/{}, tbr: {}/{}, tbc: {}/{}, tbn: {}/{}",
           decoder_ctx->width, decoder_ctx->height, decoder_fmt_ctx->video_codec->name,
           av_get_pix_fmt_name(decoder_ctx->pix_fmt),
           decoder_fmt_ctx->streams[video_stream_idx]->avg_frame_rate.num, decoder_fmt_ctx->streams[video_stream_idx]->avg_frame_rate.den,
           decoder_fmt_ctx->streams[video_stream_idx]->r_frame_rate.num, decoder_fmt_ctx->streams[video_stream_idx]->r_frame_rate.den,
           decoder_ctx->time_base.num, decoder_ctx->time_base.den,
           decoder_fmt_ctx->streams[video_stream_idx]->time_base.num, decoder_fmt_ctx->streams[video_stream_idx]->time_base.den);

    //
    // output
    //
    AVFormatContext * encoder_fmt_ctx = nullptr;
    CHECK(avformat_alloc_output_context2(&encoder_fmt_ctx, nullptr, nullptr, out_filename.c_str()) >= 0);

    // encoder
    auto encoder = avcodec_find_encoder_by_name(parser.get<std::string>("encoder", "h264_nvenc").c_str());
    CHECK(encoder);

    auto encoder_ctx = avcodec_alloc_context3(encoder);
    CHECK(encoder_ctx);
    encoder_ctx->codec_type = AVMEDIA_TYPE_VIDEO;

    if(!(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        CHECK(avio_open(&encoder_fmt_ctx->pb, out_filename.c_str(), AVIO_FLAG_WRITE) >= 0);
    }

    bool initialized = false;

    AVPacket * in_packet = av_packet_alloc();
    AVPacket * out_packet = av_packet_alloc();
    AVFrame * in_frame = av_frame_alloc();
    int64_t counter = 0;
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
                LOG(ERROR) << "error while decoding";
                return ret;
            }

            if (!initialized) {
                encoder_ctx->hw_frames_ctx = av_buffer_ref(in_frame->hw_frames_ctx);
                CHECK(encoder_ctx->hw_frames_ctx);

                encoder_ctx->hw_device_ctx = av_buffer_ref(device_ref);
                CHECK(encoder_ctx->hw_device_ctx);

                // encoder codec params
                encoder_ctx->height = decoder_ctx->height;
                encoder_ctx->width = decoder_ctx->width;
                encoder_ctx->pix_fmt = AV_PIX_FMT_CUDA;
                encoder_ctx->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;
                // time base
                encoder_ctx->time_base = av_inv_q(decoder_ctx->framerate);
                encoder_ctx->framerate = decoder_ctx->framerate;

                CHECK(avcodec_open2(encoder_ctx, encoder, nullptr) >= 0);
                CHECK(avformat_new_stream(encoder_fmt_ctx, encoder));
                CHECK(avcodec_parameters_from_context(encoder_fmt_ctx->streams[0]->codecpar, encoder_ctx) >= 0);
                encoder_fmt_ctx->streams[0]->time_base = av_inv_q(decoder_ctx->framerate);
                encoder_fmt_ctx->streams[0]->avg_frame_rate = decoder_ctx->framerate;

                CHECK(avformat_write_header(encoder_fmt_ctx, nullptr) >= 0);

                av_dump_format(encoder_fmt_ctx, 0, out_filename.c_str(), 1);
                LOG(INFO) << fmt::format("[OUTPUT] {}x{}, encoder: {}, format: {}, framerate: {}/{}, tbc: {}/{}, tbn: {}/{}",
                                         encoder_ctx->width, encoder_ctx->height, encoder->name,
                                         av_get_pix_fmt_name(encoder_ctx->pix_fmt),
                                         encoder_ctx->framerate.num, encoder_ctx->framerate.den,
                                         encoder_ctx->time_base.num, encoder_ctx->time_base.den,
                                         encoder_fmt_ctx->streams[0]->time_base.num, encoder_fmt_ctx->streams[0]->time_base.den);

                initialized = true;
            }

            // clear
            in_frame->pict_type = AV_PICTURE_TYPE_NONE;

            in_frame->pts = av_rescale_q(in_frame->pts,
                                       decoder_fmt_ctx->streams[video_stream_idx]->time_base,
                                       encoder_ctx->time_base);

            ret = avcodec_send_frame(encoder_ctx, in_frame);
            while(ret >= 0) {
                av_packet_unref(out_packet);
                ret = avcodec_receive_packet(encoder_ctx, out_packet);
                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if(ret < 0) {
                    LOG(ERROR) << "decoding error";
                    return ret;
                }

                out_packet->stream_index = 0;
                av_packet_rescale_ts(out_packet,
                                     encoder_ctx->time_base,
                                     encoder_fmt_ctx->streams[0]->time_base);

                LOG(INFO) << fmt::format(" -- [ENCODING] frame = {}, pts = {}, dts = {}",
                                         encoder_ctx->frame_number, out_packet->pts, out_packet->dts);

                if (av_interleaved_write_frame(encoder_fmt_ctx, out_packet) != 0) {
                    LOG(ERROR) << "encoder: av_interleaved_write_frame()";
                    return -1;
                }
            }
        }
        av_packet_unref(in_packet);
    }

    LOG(INFO) << fmt::format("decoded frames: {}, encoded frames: {}",
                             decoder_ctx->frame_number, encoder_ctx->frame_number);

    // flush encoders, omit

    av_packet_free(&in_packet);
    av_packet_free(&out_packet);
    av_frame_free(&in_frame);

    av_write_trailer(encoder_fmt_ctx);
    if (encoder_fmt_ctx && !(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&encoder_fmt_ctx->pb);

    avformat_close_input(&decoder_fmt_ctx);
    avformat_free_context(encoder_fmt_ctx);

    avcodec_free_context(&decoder_ctx);
    avcodec_free_context(&encoder_ctx);

    return 0;
}