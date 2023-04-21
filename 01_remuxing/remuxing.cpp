extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
}
#include <map>

int main(int argc, char* argv[])
{
    if (argc < 3) {
        printf("remuxing <input> <output>\n");
        return -1;
    }

    const char * in_filename = argv[1];
    const char * out_filename = argv[2];

    // input
    AVFormatContext* decoder_fmt_ctx = nullptr;
    if (avformat_open_input(&decoder_fmt_ctx, in_filename, nullptr, nullptr) < 0) {
        fprintf(stderr, "failed to open the %s file.\n", in_filename);
        return -1;
    }

    if (avformat_find_stream_info(decoder_fmt_ctx, nullptr) < 0) {
        fprintf(stderr, "not find stream info.\n");
        return -1;
    }

    av_dump_format(decoder_fmt_ctx, 0, in_filename, 0);

    // output
    AVFormatContext * encoder_fmt_ctx = nullptr;
    if (avformat_alloc_output_context2(&encoder_fmt_ctx, nullptr, nullptr, out_filename) < 0) {
        fprintf(stderr, "failed to alloc output-context memory.\n");
        return -1;
    }

    // map streams
    //
    // a media file always contains several streams, like video and audio streams.
    std::map<unsigned int, int> stream_mapping{};
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
            fprintf(stderr, "failed to create a stream for output.\n");
            return -1;
        }

        if (avcodec_parameters_copy(encode_stream->codecpar, decode_params) < 0) {
            fprintf(stderr, "failed to copy parameters.\n");
            return -1;
        }
    }

    // open the output file
    if(!(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&encoder_fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0) {
            printf("can not open the output file : %s.\n", out_filename);
            return -1;
        }
    }

    // header
    if(avformat_write_header(encoder_fmt_ctx, nullptr) < 0) {
        fprintf(stderr, "can not write header to the output file.\n");
        return -1;
    }

    av_dump_format(encoder_fmt_ctx, 0, out_filename, 1);

    // copy streams
    AVPacket * packet = av_packet_alloc();
    int64_t frame_number = 0;
    while(av_read_frame(decoder_fmt_ctx, packet) >= 0) {
        if (stream_mapping[packet->stream_index] < 0) {
            // the packet is reference-counted.
            // the ffmpeg is a C library, we need unreference the buffer manually
            av_packet_unref(packet);
            continue;
        }

        // convert the timestamps from the time base of input stream to the time base of output stream
        //
        // In simple terms, a time base is a rescaling of the unit second, like 1/100s, 1001/24000s, and
        // each stream has its own time base. The timestamps of the packets are based on the time base 
        // of their streams, timestamps of the same value have different meanings depending on the time unit.
        //
        // It is not necessary in this example since the time base of input and output streams is the same.
        av_packet_rescale_ts(packet, 
                             decoder_fmt_ctx->streams[packet->stream_index]->time_base, 
                             encoder_fmt_ctx->streams[stream_mapping[packet->stream_index]]->time_base);

        printf(" -- %s] pts: %6ld, dts: %6ld, duration: %5ld, frame = %6ld\n",
               av_get_media_type_string(encoder_fmt_ctx->streams[packet->stream_index]->codecpar->codec_type),
               packet->pts, packet->dts, packet->duration, frame_number++);

        packet->stream_index = stream_mapping[packet->stream_index];
        // write the packet to the output file
        // this function will take the owership of the packet, and packet will be blank
        if (av_interleaved_write_frame(encoder_fmt_ctx, packet) != 0) {
            fprintf(stderr, "failed to write frame.\n");
            return -1;
        }
    }

    // write the trailer to the output file and close it
    av_write_trailer(encoder_fmt_ctx);
    if (encoder_fmt_ctx && !(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&encoder_fmt_ctx->pb);

    // release the resources
    // let's not think about the abnormal exit for simplicity
    av_packet_free(&packet);
    avformat_close_input(&decoder_fmt_ctx);
    avformat_free_context(encoder_fmt_ctx);

    return 0;
}