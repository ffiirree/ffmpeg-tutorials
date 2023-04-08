extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}
#include "logging.h"
#include "fmt/format.h"
#include "defer.h"

enum AVPixelFormat get_hw_format(AVCodecContext *, const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
        if (*p == AV_PIX_FMT_CUDA)
            return *p;
    }

    LOG(ERROR) << "Failed to get HW surface format.";
    return AV_PIX_FMT_NONE;
}

int main(int argc, char* argv[])
{
    Logger::init(argv);

    enum AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    while((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
        LOG(INFO) << "support hwdevice: " << av_hwdevice_get_type_name(type);
    }

    CHECK(av_hwdevice_find_type_by_name("cuda") != AV_HWDEVICE_TYPE_NONE);

    CHECK(argc >= 3) << "Usage: cuda_decode <input> <output>";

    const char * in_filename = argv[1];
    const char * out_filename = argv[2];

    AVFormatContext * decoder_fmt_ctx = nullptr;
    CHECK(avformat_open_input(&decoder_fmt_ctx, in_filename, nullptr, nullptr) >= 0);
    defer(avformat_close_input(&decoder_fmt_ctx));
    CHECK(avformat_find_stream_info(decoder_fmt_ctx, nullptr) >= 0);

    int video_stream_idx = av_find_best_stream(decoder_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    CHECK(video_stream_idx >= 0);

    // decoder
    auto decoder = avcodec_find_decoder(decoder_fmt_ctx->streams[video_stream_idx]->codecpar->codec_id);
    CHECK_NOTNULL(decoder);

    enum AVPixelFormat hw_pix_fmt = AV_PIX_FMT_CUDA;
    for(int i =0;;i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(decoder, i);
        CHECK_NOTNULL(config);

        if(config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == AV_HWDEVICE_TYPE_CUDA) {
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }

    LOG(INFO) << "hardware pixel format : " << av_get_pix_fmt_name(hw_pix_fmt);

    // decoder context
    AVCodecContext * decoder_ctx = avcodec_alloc_context3(decoder);
    CHECK_NOTNULL(decoder_ctx);
    defer(avcodec_free_context(&decoder_ctx));

    CHECK(avcodec_parameters_to_context(decoder_ctx, decoder_fmt_ctx->streams[video_stream_idx]->codecpar) >= 0);

    decoder_ctx->get_format = get_hw_format;

    AVBufferRef *hw_device_ctx = nullptr;
    CHECK(av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0) >= 0);
    decoder_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    defer(av_buffer_unref(&hw_device_ctx));

    CHECK(avcodec_open2(decoder_ctx, decoder, nullptr) >= 0);

    av_dump_format(decoder_fmt_ctx, 0, in_filename, 0);
    LOG(INFO) << fmt::format("[ INPUT] {}x{}, format: {}, fps: {}/{}, tbr: {}/{}, tbc: {}/{}, tbn: {}/{}",
                             decoder_ctx->width, decoder_ctx->height,
                             av_get_pix_fmt_name(decoder_ctx->pix_fmt),
                             decoder_fmt_ctx->streams[video_stream_idx]->avg_frame_rate.num, decoder_fmt_ctx->streams[video_stream_idx]->avg_frame_rate.den,
                             decoder_fmt_ctx->streams[video_stream_idx]->r_frame_rate.num, decoder_fmt_ctx->streams[video_stream_idx]->r_frame_rate.den,
                             decoder_ctx->time_base.num, decoder_ctx->time_base.den,
                             decoder_fmt_ctx->streams[video_stream_idx]->time_base.num, decoder_fmt_ctx->streams[video_stream_idx]->time_base.den);

    // output @{
    AVFormatContext * encoder_fmt_ctx = nullptr;
    CHECK(avformat_alloc_output_context2(&encoder_fmt_ctx, nullptr, nullptr, out_filename) >= 0);
    defer(avformat_free_context(encoder_fmt_ctx));

    CHECK_NOTNULL(avformat_new_stream(encoder_fmt_ctx, nullptr));

    // encoder
    auto encoder = avcodec_find_encoder_by_name("libx264");
    CHECK_NOTNULL(encoder);

    AVCodecContext *encoder_ctx = avcodec_alloc_context3(encoder);
    CHECK_NOTNULL(encoder_ctx);
    defer(avcodec_free_context(&encoder_ctx));

    AVDictionary* encoder_options = nullptr;
    av_dict_free(&encoder_options);
    av_dict_set(&encoder_options, "crf", "23", AV_DICT_DONT_OVERWRITE);
    av_dict_set(&encoder_options, "threads", "auto", AV_DICT_DONT_OVERWRITE);

    // encoder codec params
    encoder_ctx->height = decoder_ctx->height;
    encoder_ctx->width = decoder_ctx->width;
    encoder_ctx->pix_fmt = AV_PIX_FMT_NV12;
    encoder_ctx->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;
    encoder_ctx->framerate = av_guess_frame_rate(decoder_fmt_ctx, decoder_fmt_ctx->streams[video_stream_idx], nullptr);

    // time base
    encoder_ctx->time_base = av_inv_q(encoder_ctx->framerate);
    encoder_fmt_ctx->streams[0]->time_base = decoder_fmt_ctx->streams[video_stream_idx]->time_base;

    CHECK(avcodec_open2(encoder_ctx, encoder, &encoder_options) >= 0);
    CHECK(avcodec_parameters_from_context(encoder_fmt_ctx->streams[0]->codecpar, encoder_ctx) >= 0);

    if(!(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        CHECK(avio_open(&encoder_fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) >= 0);
    }

    CHECK(avformat_write_header(encoder_fmt_ctx, nullptr) >= 0);

    av_dump_format(encoder_fmt_ctx, 0, out_filename, 1);
    LOG(INFO) << fmt::format("[OUTPUT] {}x{}, format: {}, framerate: {}/{}, tbc: {}/{}, tbn: {}/{}",
                             encoder_ctx->width, encoder_ctx->height,
                             av_get_pix_fmt_name(encoder_ctx->pix_fmt),
                             encoder_ctx->framerate.num, encoder_ctx->framerate.den,
                             encoder_ctx->time_base.num, encoder_ctx->time_base.den,
                             encoder_fmt_ctx->streams[0]->time_base.num, encoder_fmt_ctx->streams[0]->time_base.den);
    // @}

    AVPacket * in_packet = av_packet_alloc();  defer(av_packet_free(&in_packet));
    AVPacket * out_packet = av_packet_alloc();  defer(av_packet_free(&out_packet));
    AVFrame * in_frame = av_frame_alloc();  defer(av_frame_free(&in_frame));
    AVFrame * cpu_frame = av_frame_alloc(); defer(av_frame_free(&cpu_frame));
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
            }
            CHECK(ret >= 0);

            // GPU -> CPU
            CHECK(av_hwframe_transfer_data(cpu_frame, in_frame, 0) >= 0);
            cpu_frame->pts = in_frame->pts;
            cpu_frame->pict_type = AV_PICTURE_TYPE_NONE;

            ret = avcodec_send_frame(encoder_ctx, cpu_frame);
            while(ret >= 0) {
                av_packet_unref(out_packet);
                ret = avcodec_receive_packet(encoder_ctx, out_packet);
                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }
                CHECK(ret >= 0);

                out_packet->stream_index = 0;
                av_packet_rescale_ts(out_packet, decoder_fmt_ctx->streams[video_stream_idx]->time_base, encoder_fmt_ctx->streams[0]->time_base);
                LOG(INFO) << fmt::format("[ENCODING] frame = {:>3d}, pts = {:>7d}, dts = {:>7d}", encoder_ctx->frame_number, out_packet->pts, out_packet->dts);

                CHECK(av_interleaved_write_frame(encoder_fmt_ctx, out_packet) >= 0);
            }
        }
        av_packet_unref(in_packet);
    }

    LOG(INFO) << fmt::format("decoded frames: {}, encoded frames: {}", decoder_ctx->frame_number, encoder_ctx->frame_number);

    av_write_trailer(encoder_fmt_ctx);
    if (encoder_fmt_ctx && !(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&encoder_fmt_ctx->pb);

    return 0;
}