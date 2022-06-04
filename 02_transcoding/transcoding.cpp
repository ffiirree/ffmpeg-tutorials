extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        printf("transcoding <input> <output>");
        return -1;
    }

    const char * in_filename = argv[1];
    const char * out_filename = argv[2];

    AVFormatContext * decoder_fmt_ctx = avformat_alloc_context();
    if (!decoder_fmt_ctx) {
        fprintf(stderr, "avformat_alloc_context()\n");
        return -1;
    }

    if (avformat_open_input(&decoder_fmt_ctx, in_filename, nullptr, nullptr) < 0) {
        fprintf(stderr, "avformat_open_input()\n");
        return -1;
    }

    if (avformat_find_stream_info(decoder_fmt_ctx, nullptr) < 0) {
        fprintf(stderr, "avformat_find_stream_info()\n");
        return -1;
    }

    int video_stream_idx = av_find_best_stream(decoder_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_idx < 0) {
        fprintf(stderr, "decoder: av_find_best_stream()\n");
        return -1;
    }

    // decoder
    auto decoder = avcodec_find_decoder(decoder_fmt_ctx->streams[video_stream_idx]->codecpar->codec_id);
    if (!decoder) {
        fprintf(stderr, "decoder: avcodec_find_decoder()\n");
        return -1;
    }

    // decoder context
    AVCodecContext * decoder_ctx = avcodec_alloc_context3(decoder);
    if(!decoder_ctx) {
       fprintf(stderr, "decoder: avcodec_alloc_context3()\n");
        return -1;
    }

    if(avcodec_parameters_to_context(decoder_ctx, decoder_fmt_ctx->streams[video_stream_idx]->codecpar) < 0) {
        fprintf(stderr, "decoder: avcodec_parameters_to_context()\n");
        return -1;
    }

    if(avcodec_open2(decoder_ctx, decoder, nullptr) < 0) {
        fprintf(stderr, "decoder: avcodec_open2()\n");
        return -1;
    }

    av_dump_format(decoder_fmt_ctx, 0, in_filename, 0);
    printf("[ INPUT] %dx%d, fps: %d/%d, tbr: %d/%d, tbc: %d/%d, tbn: %d/%d\n",
           decoder_ctx->width, decoder_ctx->height,
           decoder_fmt_ctx->streams[video_stream_idx]->avg_frame_rate.num, decoder_fmt_ctx->streams[video_stream_idx]->avg_frame_rate.den,
           decoder_fmt_ctx->streams[video_stream_idx]->r_frame_rate.num, decoder_fmt_ctx->streams[video_stream_idx]->r_frame_rate.den,
           decoder_ctx->time_base.num, decoder_ctx->time_base.den,
           decoder_fmt_ctx->streams[video_stream_idx]->time_base.num, decoder_fmt_ctx->streams[video_stream_idx]->time_base.den);

    //
    // output
    //
    AVFormatContext * encoder_fmt_ctx = nullptr;
    if (avformat_alloc_output_context2(&encoder_fmt_ctx, nullptr, nullptr, out_filename) < 0) {
        fprintf(stderr, "encoder: avformat_alloc_output_context2()\n");
        return -1;
    }

    if(avformat_new_stream(encoder_fmt_ctx, nullptr) == nullptr) {
        fprintf(stderr, "encoder: avformat_new_stream()\n");
        return -1;
    }

    // encoder
    AVCodec *encoder = avcodec_find_encoder_by_name("libx264");
    if (!encoder) {
        fprintf(stderr, "encoder: avcodec_find_encoder_by_name()\n");
        return -1;
    }

    AVCodecContext *encoder_ctx = avcodec_alloc_context3(encoder);
    if (!encoder_ctx) {
        fprintf(stderr,"encoder: avcodec_alloc_context3()\n");
        return -1;
    }

    AVDictionary* encoder_options = nullptr;
    av_dict_set(&encoder_options, "crf", "23", AV_DICT_DONT_OVERWRITE);
    av_dict_set(&encoder_options, "threads", "auto", AV_DICT_DONT_OVERWRITE);

    // encoder codec params
    encoder_ctx->height = decoder_ctx->height;
    encoder_ctx->width = decoder_ctx->width;
    encoder_ctx->pix_fmt = decoder_ctx->pix_fmt;

    encoder_ctx->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;
    encoder_ctx->framerate = av_guess_frame_rate(decoder_fmt_ctx, decoder_fmt_ctx->streams[video_stream_idx], nullptr);
    
    // time base
    encoder_ctx->time_base = av_inv_q(encoder_ctx->framerate);
    encoder_fmt_ctx->streams[0]->time_base = decoder_fmt_ctx->streams[video_stream_idx]->time_base;

    if(avcodec_open2(encoder_ctx, encoder, &encoder_options) < 0) {
        fprintf(stderr,"encoder: avcodec_open2()\n");
        return -1;
    }

    if(avcodec_parameters_from_context(encoder_fmt_ctx->streams[0]->codecpar, encoder_ctx) < 0) {
        fprintf(stderr,"encoder: avcodec_parameters_from_context()\n");
        return -1;
    }

    if(!(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&encoder_fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr,"avio_open()\n");
            return -1;
        }
    }

    if(avformat_write_header(encoder_fmt_ctx, nullptr) < 0) {
        fprintf(stderr, "encoder: avformat_write_header()\n");
        return -1;
    }

    av_dump_format(encoder_fmt_ctx, 0, out_filename, 1);
    printf("[OUTPUT] %dx%d, framerate: %d/%d, tbc: %d/%d, tbn: %d/%d\n",
           encoder_ctx->width, encoder_ctx->height,
           encoder_ctx->framerate.num, encoder_ctx->framerate.den,
           encoder_ctx->time_base.num, encoder_ctx->time_base.den,
           encoder_fmt_ctx->streams[0]->time_base.num, encoder_fmt_ctx->streams[0]->time_base.den);

    AVPacket * in_packet = av_packet_alloc();
    AVPacket * out_packet = av_packet_alloc();
    AVFrame * in_frame = av_frame_alloc();
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
                fprintf(stderr, "encoder: avcodec_receive_frame() \n");
                return ret;
            }

            // clear
            in_frame->pict_type = AV_PICTURE_TYPE_NONE;

            ret = avcodec_send_frame(encoder_ctx, in_frame);
            while(ret >= 0) {
                av_packet_unref(out_packet);
                ret = avcodec_receive_packet(encoder_ctx, out_packet);
                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if(ret < 0) {
                    fprintf(stderr, "encoder: avcodec_receive_packet()\n");
                    return ret;
                }

                out_packet->stream_index = 0;
                av_packet_rescale_ts(out_packet, decoder_fmt_ctx->streams[video_stream_idx]->time_base, encoder_fmt_ctx->streams[0]->time_base);
                printf(" -- [ENCODING] frame = %d, pts = %lld, dts = %lld, duration = %lld\n", encoder_ctx->frame_number, out_packet->pts, out_packet->dts, out_packet->duration);

                if (av_interleaved_write_frame(encoder_fmt_ctx, out_packet) != 0) {
                    fprintf(stderr, "encoder: av_interleaved_write_frame()\n");
                    return -1;
                }
            }
        }
        av_packet_unref(in_packet);
    }

    printf("decoded frames: %d, encoded frames: %d\n", decoder_ctx->frame_number, encoder_ctx->frame_number);

    av_packet_free(&in_packet);
    av_packet_free(&out_packet);
    av_frame_free(&in_frame);

    av_write_trailer(encoder_fmt_ctx);
    if (encoder_fmt_ctx && !(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&encoder_fmt_ctx->pb);

    av_dict_free(&encoder_options);

    avformat_close_input(&decoder_fmt_ctx);
    avformat_free_context(encoder_fmt_ctx);

    avcodec_free_context(&decoder_ctx);
    avcodec_free_context(&encoder_ctx);

	return 0;
}