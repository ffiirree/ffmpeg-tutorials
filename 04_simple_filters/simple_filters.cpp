extern "C" {
#include <libavformat\avformat.h>
#include <libavcodec\avcodec.h>
#include <libavutil\opt.h>
#include <libavutil\time.h>
#include <libavfilter\avfilter.h>
#include <libavfilter\buffersink.h>
#include <libavfilter\buffersrc.h>
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        printf("filtering <input> <output>");
        return -1;
    }

    const char * in_filename = argv[1];
    const char * out_filename = argv[2];

    auto decoder_fmt_ctx = avformat_alloc_context();

    if (avformat_open_input(&decoder_fmt_ctx, in_filename, nullptr, nullptr) < 0) {
        fprintf(stderr, "avformat_open_input(%s)\n", in_filename);
        return -1;
    }

    if (avformat_find_stream_info(decoder_fmt_ctx, nullptr) < 0) {
        fprintf(stderr, "avformat_find_stream_info()\n");
        return -1;
    }

    av_dump_format(decoder_fmt_ctx, 0, in_filename, 0);

    int video_stream_idx = av_find_best_stream(decoder_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_idx < 0) {
        fprintf(stderr, "av_find_best_stream()\n");
        return -1;
    }

    // decoder
    auto decoder = avcodec_find_decoder(decoder_fmt_ctx->streams[video_stream_idx]->codecpar->codec_id);
    if (!decoder) {
        fprintf(stderr, "avcodec_find_decoder()\n");
        return -1;
    }

    // decoder context
    auto decoder_ctx = avcodec_alloc_context3(decoder);
    if(!decoder_ctx) {
        fprintf(stderr, "decoder: avcodec_alloc_context3()\n");
        return -1;
    }

    if(avcodec_parameters_to_context(decoder_ctx, decoder_fmt_ctx->streams[video_stream_idx]->codecpar) < 0) {
        fprintf(stderr, "decoder: avcodec_parameters_to_context()\n");
        return -1;
    }

    AVDictionary * decoder_options = nullptr;
    av_dict_set(&decoder_options, "threads", "auto", AV_DICT_DONT_OVERWRITE);
    if(avcodec_open2(decoder_ctx, decoder, &decoder_options) < 0) {
        fprintf(stderr, "decoder: avcodec_open2()\n");
        return -1;
    }

    //
    // output
    //
    AVFormatContext * encoder_fmt_ctx = nullptr;
    if (avformat_alloc_output_context2(&encoder_fmt_ctx, nullptr, nullptr, out_filename) < 0) {
        fprintf(stderr, "avformat_alloc_output_context2\n");
        return -1;
    }

    if(avformat_new_stream(encoder_fmt_ctx, nullptr) == nullptr) {
        fprintf(stderr, "avformat_new_stream\n");
        return -1;
    }

    // encoder
    AVCodec *encoder = avcodec_find_encoder_by_name("libx264");
    if (!encoder) {
        fprintf(stderr, "avcodec_find_encoder_by_name\n");
        return -1;
    }
    AVCodecContext *encoder_ctx = avcodec_alloc_context3(encoder);
    if (!encoder_ctx) {
        printf("avcodec_alloc_context3\n");
        return -1;
    }

    AVDictionary* encoder_options = nullptr;
    av_dict_set(&encoder_options, "preset", "ultrafast", AV_DICT_DONT_OVERWRITE);
    av_dict_set(&encoder_options, "tune", "zerolatency", AV_DICT_DONT_OVERWRITE);
    av_dict_set(&encoder_options, "crf", "23", AV_DICT_DONT_OVERWRITE);
    av_dict_set(&encoder_options, "threads", "auto", AV_DICT_DONT_OVERWRITE);

    // encoder codec params
    encoder_ctx->height = decoder_ctx->height;
    encoder_ctx->width = decoder_ctx->width;
    encoder_ctx->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;
    encoder_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    // time base
    encoder_ctx->time_base = decoder_fmt_ctx->streams[video_stream_idx]->time_base;
    encoder_fmt_ctx->streams[0]->time_base = encoder_ctx->time_base;
    encoder_ctx->framerate = av_guess_frame_rate(decoder_fmt_ctx, decoder_fmt_ctx->streams[video_stream_idx], nullptr);

    if ( encoder_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        encoder_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if(avcodec_open2(encoder_ctx, encoder, &encoder_options) < 0) {
        printf("encoder: avcodec_open2()\n");
        return -1;
    }

    if(avcodec_parameters_from_context(encoder_fmt_ctx->streams[0]->codecpar, encoder_ctx) < 0) {
        printf("encoder: avcodec_parameters_from_context()\n");
        return -1;
    }

    if(!(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&encoder_fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0) {
            printf("avio_open()\n");
            return -1;
        }
    }

    if(avformat_write_header(encoder_fmt_ctx, nullptr) < 0) {
        fprintf(stderr, "avformat_write_header()\n");
        return -1;
    }

    av_dump_format(encoder_fmt_ctx, 0, out_filename, 1);

    // filters @{
    AVFilterGraph * filter_graph = avfilter_graph_alloc();
    if(!filter_graph) {
        fprintf(stderr, "avfilter_graph_alloc()\n");
        return -1;
    }

    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    if (!buffersrc) {
        fprintf(stderr, "avfilter_get_by_name(\"buffer\")\n");
        return -1;
    }
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    if (!buffersink) {
        fprintf(stderr, "avfilter_get_by_name(\"buffersink\")\n");
        return -1;
    }
    const AVFilter *vflip_filter = avfilter_get_by_name("vflip");
    if (!vflip_filter) {
        fprintf(stderr, "avfilter_get_by_name(\"vflip\")\n");
        return -1;
    }

    AVFilterContext *in_filter_ctx = nullptr;
    AVFilterContext *out_filter_ctx = nullptr;
    AVFilterContext *vflip_filter_ctx = nullptr;
    char args[512] {0};
    sprintf(args, "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d",
            decoder_ctx->width, decoder_ctx->height,
            decoder_ctx->pix_fmt,
            decoder_fmt_ctx->streams[video_stream_idx]->time_base.num, decoder_fmt_ctx->streams[video_stream_idx]->time_base.den,
            decoder_fmt_ctx->streams[video_stream_idx]->sample_aspect_ratio.num, decoder_fmt_ctx->streams[video_stream_idx]->sample_aspect_ratio.den);
    printf("filter@buffer[args]: %s\n", args);

    if (avfilter_graph_create_filter(&in_filter_ctx, buffersrc, "in", args, nullptr, filter_graph) < 0) {
        fprintf(stderr, "avfilter_graph_create_filter(&in_filter_ctx)\n");
        return -1;
    }
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    if (avfilter_graph_create_filter(&out_filter_ctx, buffersink, "sink", nullptr, nullptr, filter_graph) < 0) {
        fprintf(stderr, "avfilter_graph_create_filter(&out_filter_ctx)\n");
        return -1;
    }
    if(av_opt_set_int_list(out_filter_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN) < 0) {
        fprintf(stderr, "av_opt_set_int_list()\n");
        return -1;
    }
    if (avfilter_graph_create_filter(&vflip_filter_ctx, vflip_filter, "vflip", nullptr, nullptr, filter_graph) < 0) {
        fprintf(stderr, "avfilter_graph_create_filter(&vflip_filter_ctx)\n");
        return -1;
    }

    if(avfilter_link(in_filter_ctx, 0, vflip_filter_ctx, 0) != 0) {
        fprintf(stderr, "avfilter_link(in_filter_ctx, 0, vflip_filter_ctx, 0)\n");
        return -1;
    }
    if(avfilter_link(vflip_filter_ctx, 0, out_filter_ctx, 0) != 0) {
        fprintf(stderr, "avfilter_link(vflip_filter_ctx, 0, out_filter_ctx, 0)\n");
        return -1;
    }

    if(avfilter_graph_config(filter_graph, nullptr) < 0) {
        fprintf(stderr, "avfilter_graph_config()\n");
        return -1;
    }

    char * graph = avfilter_graph_dump(filter_graph, nullptr);
    if (graph) {
        printf("graph >>>> \n%s\n", graph);
    } else {
        fprintf(stderr, "avfilter_graph_dump()\n");
        return -1;
    }
    // @}

    int64_t first_pts = 0;
    AVPacket * in_packet = av_packet_alloc();
    AVFrame * in_frame = av_frame_alloc();
    AVPacket* out_packet = av_packet_alloc();
    AVFrame * filtered_frame = av_frame_alloc();
    if(!filtered_frame) {
        fprintf(stderr, "av_frame_alloc(\"filtered_frame\")\n");
        return -1;
    }
    while(av_read_frame(decoder_fmt_ctx, in_packet)) {
        if (in_packet->stream_index != video_stream_idx) {
            continue;
        }

        int ret = avcodec_send_packet(decoder_ctx, in_packet);
        while(ret >= 0) {
            ret = avcodec_receive_frame(decoder_ctx, in_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                fprintf(stderr, "encoder: avcodec_receive_frame() \n");
                return ret;
            }

            if(av_buffersrc_add_frame_flags(in_filter_ctx, in_frame, AV_BUFFERSRC_FLAG_PUSH) < 0) {
                fprintf(stderr, "av_buffersrc_add_frame_flags()\n");
                break;
            }

            while(true) {
                ret = av_buffersink_get_frame_flags(out_filter_ctx, filtered_frame, AV_BUFFERSINK_FLAG_NO_REQUEST);

                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                if (ret < 0) {
                    fprintf(stderr, "av_buffersink_get_frame_flags()\n");
                    return ret;
                }

                // encode
                if (ret >= 0) {
                    ret = avcodec_send_frame(encoder_ctx, filtered_frame);
                    while(ret >= 0) {
                        ret = avcodec_receive_packet(encoder_ctx, out_packet);

                        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                            break;
                        } else if(ret < 0) {
                            fprintf(stderr, "encoder: avcodec_receive_packet()\n");
                            return -1;
                        }

                        out_packet->stream_index = 0;
                        av_packet_rescale_ts(out_packet, decoder_fmt_ctx->streams[video_stream_idx]->time_base, encoder_fmt_ctx->streams[0]->time_base);

                        if (av_interleaved_write_frame(encoder_fmt_ctx, out_packet) != 0) {
                            fprintf(stderr, "encoder: av_interleaved_write_frame()\n");
                            return -1;
                        }
                    }
                    av_packet_unref(out_packet);
                }
                av_frame_unref(filtered_frame);
            }
            av_frame_unref(in_frame);
        }
        av_packet_unref(in_packet);
    }

    av_packet_free(&in_packet);
    av_frame_free(&in_frame);
    av_packet_free(&out_packet);
    av_frame_free(&filtered_frame);

    av_write_trailer(encoder_fmt_ctx);

    avformat_close_input(&decoder_fmt_ctx);
    avformat_free_context(encoder_fmt_ctx);

    avcodec_free_context(&decoder_ctx);
    avcodec_free_context(&encoder_ctx);

    av_dict_free(&decoder_options);
    av_dict_free(&encoder_options);

    return 0;
}