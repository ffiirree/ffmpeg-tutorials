extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavdevice/avdevice.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

int main(int argc, char* argv[])
{
    if (argc < 4) {
        printf("recording <format(dshow/gdigrab)> <input(video=CAMERA/desktop)> <output>");
        printf("\trecording dshow video=\"HD WebCam\" camera.mp4");
        printf("\trecording gdigrab desktop desktop.mp4");
        return -1;
    }

    const char * input_format = argv[1];
    const char * input = argv[2];
    const char * out_filename = argv[3];

    avdevice_register_all();

    AVFormatContext * decoder_fmt_ctx = avformat_alloc_context();
    if (!decoder_fmt_ctx) {
        fprintf(stderr, "avformat_alloc_context()\n");
        return -1;
    }

    if (avformat_open_input(&decoder_fmt_ctx, input, av_find_input_format(input_format), nullptr) < 0) {
        fprintf(stderr, "avformat_open_input(%s)\n", input);
        return -1;
    }

    if (avformat_find_stream_info(decoder_fmt_ctx, nullptr) < 0) {
        fprintf(stderr, "avformat_find_stream_info()\n");
        return -1;
    }

    av_dump_format(decoder_fmt_ctx, 0, input, 0);

    int video_stream_idx = av_find_best_stream(decoder_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_idx < 0) {
        fprintf(stderr, "decoder: av_find_best_stream(AVMEDIA_TYPE_VIDEO)\n");
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
    AVCodec *encoder = avcodec_find_encoder_by_name("libx265");
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
    encoder_ctx->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;
    encoder_ctx->pix_fmt = encoder->pix_fmts[0];
    encoder_ctx->framerate = av_guess_frame_rate(decoder_fmt_ctx, decoder_fmt_ctx->streams[video_stream_idx], nullptr);

    // time base
    encoder_ctx->time_base = av_inv_q(encoder_ctx->framerate);
    encoder_fmt_ctx->streams[0]->time_base = encoder_ctx->time_base;

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

    AVPacket * in_packet = av_packet_alloc();
    AVPacket * out_packet = av_packet_alloc();
    AVFrame * decoded_frame = av_frame_alloc();

    SwsContext * sws_ctx = sws_getContext(
            decoder_ctx->width,decoder_ctx->height,decoder_ctx->pix_fmt,
            encoder_ctx->width,encoder_ctx->height,encoder_ctx->pix_fmt,
            SWS_BICUBIC, nullptr, nullptr, nullptr
    );
    if (!sws_ctx) {
        fprintf(stderr, "sws_getContext()\n");
        return -1;
    }
    AVFrame * scaled_frame = av_frame_alloc();
    if(av_image_alloc(scaled_frame->data, scaled_frame->linesize,
                      encoder_ctx->width, encoder_ctx->height,
                      encoder_ctx->pix_fmt, 8) < 0) {
        fprintf(stderr, "av_image_alloc()\n");
        return -1;
    }

    printf("encoder: framerate: %d/%d, context timebase: %d/%d, stream timebase: %d/%d\n",
           encoder_ctx->framerate.num, encoder_ctx->framerate.den,
           encoder_ctx->time_base.num, encoder_ctx->time_base.den,
           encoder_fmt_ctx->streams[0]->time_base.num, encoder_fmt_ctx->streams[0]->time_base.den);

    int64_t first_pts = AV_NOPTS_VALUE;
    while(av_read_frame(decoder_fmt_ctx, in_packet) >= 0 && decoder_ctx->frame_number < 200) {
        if (in_packet->stream_index != video_stream_idx) {
            continue;
        }

        int ret = avcodec_send_packet(decoder_ctx, in_packet);
        while(ret >= 0) {
            av_frame_unref(decoded_frame);
            ret = avcodec_receive_frame(decoder_ctx, decoded_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                fprintf(stderr, "encoder: avcodec_receive_frame() \n");
                return ret;
            }

            sws_scale(sws_ctx,
                      static_cast<const uint8_t *const *>(decoded_frame->data), decoded_frame->linesize,
                      0, decoder_ctx->height,
                      scaled_frame->data, scaled_frame->linesize);
            scaled_frame->height = decoded_frame->height;
            scaled_frame->width = decoded_frame->width;
            scaled_frame->format = encoder_ctx->pix_fmt;

            // manually
            first_pts = first_pts == AV_NOPTS_VALUE ? av_gettime_relative() : first_pts;
            scaled_frame->pts = av_rescale_q(av_gettime_relative() - first_pts, { 1, AV_TIME_BASE }, encoder_ctx->time_base);

            ret = avcodec_send_frame(encoder_ctx, scaled_frame);
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
                av_packet_rescale_ts(out_packet, encoder_ctx->time_base, encoder_fmt_ctx->streams[0]->time_base);

                printf(" -- [ENCODING] frame = %d, pts: %lld, dts: %lld, duration: %lld, size = %d\n",
                       encoder_ctx->frame_number, out_packet->pts, out_packet->dts, out_packet->duration, out_packet->size);

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
    av_frame_free(&decoded_frame);
    av_frame_free(&scaled_frame);

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