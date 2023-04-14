extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavdevice/avdevice.h>
}
#include "logging.h"
#include "defer.h"
#include "fmt/format.h"

int main(int argc, char* argv[])
{
    Logger::init(argv);

    CHECK(argc >= 3) << "filter <input> <output>";

    const char * in_filename = argv[1];
    const char * out_filename = argv[2];

    AVFormatContext* decoder_fmt_ctx = nullptr;
    CHECK(avformat_open_input(&decoder_fmt_ctx, in_filename, nullptr, nullptr) >= 0);
    defer(avformat_close_input(&decoder_fmt_ctx));

    CHECK(avformat_find_stream_info(decoder_fmt_ctx, nullptr) >= 0);
    int video_stream_idx = av_find_best_stream(decoder_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    CHECK(video_stream_idx >= 0);

    // decoder
    auto decoder = avcodec_find_decoder(decoder_fmt_ctx->streams[video_stream_idx]->codecpar->codec_id);
    CHECK_NOTNULL(decoder);

    // decoder context
    AVCodecContext* decoder_ctx = avcodec_alloc_context3(decoder);
    defer(avcodec_free_context(&decoder_ctx));
    CHECK_NOTNULL(decoder_ctx);

    CHECK(avcodec_parameters_to_context(decoder_ctx, decoder_fmt_ctx->streams[video_stream_idx]->codecpar) >= 0);

    AVDictionary * decoder_options = nullptr;
    av_dict_set(&decoder_options, "threads", "auto", AV_DICT_DONT_OVERWRITE);
    defer(av_dict_free(&decoder_options));

    CHECK(avcodec_open2(decoder_ctx, decoder, &decoder_options) >= 0);

    av_dump_format(decoder_fmt_ctx, 0, in_filename, 0);
    LOG(INFO) << fmt::format("[ INPUT] {:>3d}x{:>3d}, fps = {}/{}, tbr = {}/{}, tbc = {}/{}, tbn = {}/{}",
                             decoder_ctx->width, decoder_ctx->height,
                             decoder_fmt_ctx->streams[video_stream_idx]->avg_frame_rate.num, decoder_fmt_ctx->streams[video_stream_idx]->avg_frame_rate.den,
                             decoder_fmt_ctx->streams[video_stream_idx]->r_frame_rate.num, decoder_fmt_ctx->streams[video_stream_idx]->r_frame_rate.den,
                             decoder_ctx->time_base.num, decoder_ctx->time_base.den,
                             decoder_fmt_ctx->streams[video_stream_idx]->time_base.num, decoder_fmt_ctx->streams[video_stream_idx]->time_base.den);

    // filters @{
    AVFilterGraph * filter_graph = avfilter_graph_alloc();
    CHECK_NOTNULL(filter_graph);
    defer(avfilter_graph_free(&filter_graph));

    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    CHECK_NOTNULL(buffersrc);
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    CHECK_NOTNULL(buffersink);
    const AVFilter *vflip_filter = avfilter_get_by_name("vflip");
    CHECK_NOTNULL(vflip_filter);

    AVStream* video_stream = decoder_fmt_ctx->streams[video_stream_idx];
    AVRational fr = av_guess_frame_rate(decoder_fmt_ctx, video_stream, nullptr);
    std::string args = fmt::format("video_size={}x{}:pix_fmt={}:time_base={}/{}:pixel_aspect={}/{}:frame_rate={}/{}",
                                   decoder_ctx->width, decoder_ctx->height, decoder_ctx->pix_fmt,
                                   video_stream->time_base.num, video_stream->time_base.den,
                                   video_stream->codecpar->sample_aspect_ratio.num, std::max<int>(1, video_stream->codecpar->sample_aspect_ratio.den),
                                   fr.num, fr.den);

    LOG(INFO) << "buffersrc args: " << args;

    AVFilterContext *src_filter_ctx = nullptr;
    AVFilterContext *sink_filter_ctx = nullptr;
    AVFilterContext *vflip_filter_ctx = nullptr;

    CHECK(avfilter_graph_create_filter(&src_filter_ctx, buffersrc, "src", args.c_str(), nullptr, filter_graph) >= 0);
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    CHECK(avfilter_graph_create_filter(&sink_filter_ctx, buffersink, "sink", nullptr, nullptr, filter_graph) >= 0);
    CHECK(av_opt_set_int_list(sink_filter_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) >= 0);
    CHECK (avfilter_graph_create_filter(&vflip_filter_ctx, vflip_filter, "vflip", nullptr, nullptr, filter_graph) >= 0);

    CHECK(avfilter_link(src_filter_ctx, 0, vflip_filter_ctx, 0) == 0);
    CHECK(avfilter_link(vflip_filter_ctx, 0, sink_filter_ctx, 0) == 0);

    CHECK(avfilter_graph_config(filter_graph, nullptr) >= 0);
    char * graph = avfilter_graph_dump(filter_graph, nullptr);
    CHECK_NOTNULL(graph);
    LOG(INFO) << "filter graph >>>> \n" << graph;
    // @}

    LOG(INFO) << fmt::format("[FILTER] {:>3d}x{:>3d}, framerate = {}/{}, timebase = {}/{}",
                             av_buffersink_get_w(sink_filter_ctx), av_buffersink_get_h(sink_filter_ctx),
                             av_buffersink_get_frame_rate(sink_filter_ctx).num, av_buffersink_get_frame_rate(sink_filter_ctx).den,
                             av_buffersink_get_time_base(sink_filter_ctx).num, av_buffersink_get_time_base(sink_filter_ctx).den);

    //
    // output
    //
    AVFormatContext * encoder_fmt_ctx = nullptr;
    CHECK(avformat_alloc_output_context2(&encoder_fmt_ctx, nullptr, nullptr, out_filename) >= 0);
    defer(avformat_free_context(encoder_fmt_ctx));

    CHECK_NOTNULL(avformat_new_stream(encoder_fmt_ctx, nullptr));

    // encoder
    auto encoder = avcodec_find_encoder_by_name("libx265");
    CHECK_NOTNULL(encoder);

    AVCodecContext *encoder_ctx = avcodec_alloc_context3(encoder);
    CHECK_NOTNULL(encoder_ctx);
    defer(avcodec_free_context(&encoder_ctx));

    AVDictionary* encoder_options = nullptr;
    av_dict_set(&encoder_options, "crf", "23", AV_DICT_DONT_OVERWRITE);
    av_dict_set(&encoder_options, "threads", "auto", AV_DICT_DONT_OVERWRITE);
    defer(av_dict_free(&encoder_options));

    // encoder codec params
    encoder_ctx->height = decoder_ctx->height;
    encoder_ctx->width = decoder_ctx->width;
    encoder_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    encoder_ctx->sample_aspect_ratio = av_buffersink_get_sample_aspect_ratio(sink_filter_ctx);
    encoder_ctx->framerate = av_buffersink_get_frame_rate(sink_filter_ctx);

    // time base
    encoder_ctx->time_base = av_inv_q(encoder_ctx->framerate);
    encoder_fmt_ctx->streams[0]->time_base = av_buffersink_get_time_base(sink_filter_ctx);

    if (encoder_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        encoder_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    CHECK(avcodec_open2(encoder_ctx, encoder, &encoder_options) >= 0);
    CHECK(avcodec_parameters_from_context(encoder_fmt_ctx->streams[0]->codecpar, encoder_ctx) >= 0);

    if(!(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        CHECK(avio_open(&encoder_fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) >= 0);
    }

    CHECK(avformat_write_header(encoder_fmt_ctx, nullptr) >= 0);

    av_dump_format(encoder_fmt_ctx, 0, out_filename, 1);
    LOG(INFO) << fmt::format("[OUTPUT] {:>3d}x{:>3d}, framerate = {}/{}, tbc = {}/{}, tbn = {}/{}",
                             encoder_ctx->width, encoder_ctx->height,
                             encoder_ctx->framerate.num, encoder_ctx->framerate.den,
                             encoder_ctx->time_base.num, encoder_ctx->time_base.den,
                             encoder_fmt_ctx->streams[0]->time_base.num, encoder_fmt_ctx->streams[0]->time_base.den);

    AVPacket * in_packet = av_packet_alloc();
    AVFrame * in_frame = av_frame_alloc();
    AVPacket* out_packet = av_packet_alloc();
    AVFrame * filtered_frame = av_frame_alloc();

    while(av_read_frame(decoder_fmt_ctx, in_packet) >= 0 && decoder_ctx->frame_number < 200) {
        if (in_packet->stream_index != video_stream_idx) {
            continue;
        }

        in_packet->pts -= decoder_fmt_ctx->streams[video_stream_idx]->start_time;

        int ret = avcodec_send_packet(decoder_ctx, in_packet);
        while(ret >= 0) {
            av_frame_unref(in_frame);
            ret = avcodec_receive_frame(decoder_ctx, in_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                fprintf(stderr, "encoder: avcodec_receive_frame() \n");
                return ret;
            }

            // filtering
            if((ret = av_buffersrc_add_frame_flags(src_filter_ctx, in_frame, AV_BUFFERSRC_FLAG_PUSH)) < 0) {
                fprintf(stderr, "av_buffersrc_add_frame_flags()\n");
                break;
            }
            while(ret >= 0) {
                av_frame_unref(filtered_frame);
                ret = av_buffersink_get_frame_flags(sink_filter_ctx, filtered_frame, AV_BUFFERSINK_FLAG_NO_REQUEST);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }
                else if (ret < 0) {
                    fprintf(stderr, "av_buffersink_get_frame_flags()\n");
                    return ret;
                }

                //
                filtered_frame->pict_type = AV_PICTURE_TYPE_NONE;

                // encoding
                ret = avcodec_send_frame(encoder_ctx, filtered_frame);
                while(ret >= 0) {
                    av_packet_unref(out_packet);
                    ret = avcodec_receive_packet(encoder_ctx, out_packet);
                    if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        break;
                    } else if(ret < 0) {
                        fprintf(stderr, "encoder: avcodec_receive_packet()\n");
                        return -1;
                    }

                    out_packet->stream_index = 0;
                    av_packet_rescale_ts(out_packet, decoder_fmt_ctx->streams[video_stream_idx]->time_base, encoder_fmt_ctx->streams[0]->time_base);
                    LOG(INFO) << fmt::format("[ENCODING] frame = {:>4d}, pts = {:>6d}, dts = {:>6d}, size = {:>6d}",
                                             decoder_ctx->frame_number, out_packet->pts, out_packet->dts, out_packet->size);

                    if (av_interleaved_write_frame(encoder_fmt_ctx, out_packet) != 0) {
                        LOG(ERROR) << "encoder: av_interleaved_write_frame()";
                        return -1;
                    }
                }
            }
        }
        av_packet_unref(in_packet);
    }

    av_packet_free(&in_packet);
    av_frame_free(&in_frame);
    av_packet_free(&out_packet);
    av_frame_free(&filtered_frame);

    av_write_trailer(encoder_fmt_ctx);
    if (encoder_fmt_ctx && !(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&encoder_fmt_ctx->pb);

    return 0;
}