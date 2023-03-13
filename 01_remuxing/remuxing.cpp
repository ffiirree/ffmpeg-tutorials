extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
}
#include <map>

int main(int argc, char* argv[])
{
    if (argc < 3) {
        printf("remuxing <input> <output>");
        return -1;
    }

    const char * in_filename = argv[1];
    const char * out_filename = argv[2];

    AVFormatContext* decoder_fmt_ctx = avformat_alloc_context();
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

    av_dump_format(decoder_fmt_ctx, 0, in_filename, 0);

    //
    // output
    //
    AVFormatContext * encoder_fmt_ctx = nullptr;
    if (avformat_alloc_output_context2(&encoder_fmt_ctx, nullptr, nullptr, out_filename) < 0) {
        fprintf(stderr, "avformat_alloc_output_context2()\n");
        return -1;
    }

    std::map<unsigned int, int> stream_mapping;
    int stream_idx = 0;
    for (unsigned int i = 0; i < decoder_fmt_ctx->nb_streams; i++) {
        AVCodecParameters * decode_params = decoder_fmt_ctx->streams[i]->codecpar;
        if (decode_params->codec_type != AVMEDIA_TYPE_VIDEO &&
            decode_params->codec_type != AVMEDIA_TYPE_AUDIO &&
            decode_params->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            stream_mapping[i] = -1;
            continue;
        }

        stream_mapping[i] = stream_idx++;

        AVStream * encode_stream = avformat_new_stream(encoder_fmt_ctx, nullptr);
        if(encode_stream == nullptr) {
            fprintf(stderr, "avformat_new_stream()\n");
            return -1;
        }

        if (avcodec_parameters_copy(encode_stream->codecpar, decode_params) < 0) {
            fprintf(stderr, "avcodec_parameters_copy()\n");
            return -1;
        }
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

    AVPacket * packet = av_packet_alloc();
    int64_t frame_number = 0;
    while(av_read_frame(decoder_fmt_ctx, packet) >= 0) {
        if (stream_mapping[packet->stream_index] < 0) {
            av_packet_unref(packet);
            continue;
        }

        av_packet_rescale_ts(packet, decoder_fmt_ctx->streams[packet->stream_index]->time_base, encoder_fmt_ctx->streams[stream_mapping[packet->stream_index]]->time_base);
        packet->stream_index = stream_mapping[packet->stream_index];

        if (encoder_fmt_ctx->streams[packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            printf(" -- pts: %ld, dts: %ld, duration: %ld, frame = %ld\n",
                   packet->pts, packet->dts, packet->duration, ++frame_number);
        }

        if (av_interleaved_write_frame(encoder_fmt_ctx, packet) != 0) {
            fprintf(stderr, "encoder: av_interleaved_write_frame()\n");
            return -1;
        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);

    av_write_trailer(encoder_fmt_ctx);
    if (encoder_fmt_ctx && !(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&encoder_fmt_ctx->pb);

    avformat_close_input(&decoder_fmt_ctx);
    avformat_free_context(encoder_fmt_ctx);

    return 0;
}