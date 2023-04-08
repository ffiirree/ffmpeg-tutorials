extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

#include "defer.h"
#include "logging.h"
#include "fmt/format.h"

int main(int argc, char* argv[])
{
    Logger::init(argv);

    if (argc < 4) {
        LOG(ERROR) << "recording_mic <format(dshow/alsa/pulse)> <input(audio=MICROPHONE/default)> <output>";
        LOG(ERROR) << "\t[Windows] recording_mic dshow audio=audio\"MICROPHONE\" out.mp4";
        LOG(ERROR) << "\t[Ubuntu ] recording_mic alsa default out.mp4";
        LOG(ERROR) << "\t[Ubuntu ] recording_mic pulse default out.mp4";;
        return -1;
    }

    const char * input_format = argv[1];
    const char * input = argv[2];
    const char * out_filename = argv[3];

    avdevice_register_all();

    // INPUT @{
    AVFormatContext * decoder_fmt_ctx = nullptr;
    CHECK(avformat_open_input(&decoder_fmt_ctx, input, av_find_input_format(input_format), nullptr) >= 0);
    CHECK(avformat_find_stream_info(decoder_fmt_ctx, nullptr) >= 0);
    int audio_stream_idx = av_find_best_stream(decoder_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    CHECK(audio_stream_idx >= 0);

    // decoder
    auto decoder = avcodec_find_decoder(decoder_fmt_ctx->streams[audio_stream_idx]->codecpar->codec_id);
    CHECK_NOTNULL(decoder);

    // decoder context
    AVCodecContext * decoder_ctx = avcodec_alloc_context3(decoder);
    CHECK_NOTNULL(decoder_ctx);

    CHECK(avcodec_parameters_to_context(decoder_ctx, decoder_fmt_ctx->streams[audio_stream_idx]->codecpar) >= 0);
    CHECK(avcodec_open2(decoder_ctx, decoder, nullptr) >= 0);

    av_dump_format(decoder_fmt_ctx, 0, input, 0);
    LOG(INFO) << fmt::format("[ INPUT] codec_name = {}, sample_rate = {}, channels = {}, sample_fmt = {}, frame_size = {}",
                             decoder->name, decoder_ctx->sample_rate, decoder_ctx->channels,
                             av_get_sample_fmt_name(decoder_ctx->sample_fmt),
                             decoder_fmt_ctx->streams[audio_stream_idx]->codecpar->frame_size);
    // @}

    // OUTPUT @{
    AVFormatContext * encoder_fmt_ctx = nullptr;
    CHECK(avformat_alloc_output_context2(&encoder_fmt_ctx, nullptr, nullptr, out_filename) >= 0);

    CHECK_NOTNULL(avformat_new_stream(encoder_fmt_ctx, nullptr));

    // encoder
    auto encoder = avcodec_find_encoder_by_name("aac");
    CHECK_NOTNULL(encoder);
    AVCodecContext *encoder_ctx = avcodec_alloc_context3(encoder);
    CHECK_NOTNULL(encoder_ctx);

    // encoder codec params
    encoder_ctx->sample_rate = decoder_ctx->sample_rate;
    encoder_ctx->channels = decoder_ctx->channels;
    encoder_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    encoder_ctx->channel_layout = av_get_default_channel_layout(decoder_ctx->channels);

    // time base
    encoder_ctx->time_base = {1, encoder_ctx->sample_rate};
    encoder_fmt_ctx->streams[0]->time_base = encoder_ctx->time_base;

    CHECK(avcodec_open2(encoder_ctx, encoder, nullptr) >= 0);
    CHECK(avcodec_parameters_from_context(encoder_fmt_ctx->streams[0]->codecpar, encoder_ctx) >= 0);

    if(!(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        CHECK(avio_open(&encoder_fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) >= 0);
    }

    CHECK(avformat_write_header(encoder_fmt_ctx, nullptr) >= 0);

    SwrContext* swr_ctx = swr_alloc_set_opts(nullptr,
                                             av_get_default_channel_layout(encoder_ctx->channels),
                                             encoder_ctx->sample_fmt,
                                             encoder_ctx->sample_rate,
                                             av_get_default_channel_layout(decoder_ctx->channels),
                                             decoder_ctx->sample_fmt,
                                             decoder_ctx->sample_rate,
                                             0, nullptr);
    CHECK_NOTNULL(swr_ctx);
    CHECK(swr_init(swr_ctx) >= 0);
    defer(swr_free(&swr_ctx));

    av_dump_format(encoder_fmt_ctx, 0, out_filename, 1);
    LOG(INFO) << fmt::format("[OUTPUT] codec_name = {}, sample_rate = {}Hz, channels = {}, sample_fmt = {}, frame_size = {}, tbc = {}/{}, tbn = {}/{}",
                             encoder->name, encoder_ctx->sample_rate, encoder_ctx->channels,
                             av_get_sample_fmt_name(encoder_ctx->sample_fmt),
                             encoder_fmt_ctx->streams[0]->codecpar->frame_size,
                             encoder_ctx->time_base.num, encoder_ctx->time_base.den,
                             encoder_fmt_ctx->streams[0]->time_base.num, encoder_fmt_ctx->streams[0]->time_base.den);
    // @}

    AVPacket * in_packet = av_packet_alloc(); defer(av_packet_free(&in_packet));
    AVPacket * out_packet = av_packet_alloc(); defer(av_packet_free(&out_packet));
    AVFrame * decoded_frame = av_frame_alloc(); defer(av_frame_free(&decoded_frame));
    AVFrame * resampled_frame = av_frame_alloc(); defer(av_frame_free(&resampled_frame));

    AVAudioFifo *audio_buffer = av_audio_fifo_alloc(encoder_ctx->sample_fmt, encoder_ctx->channels, 1);
    CHECK_NOTNULL(audio_buffer);
    defer(av_audio_fifo_free(audio_buffer));

    auto alloc_frame_buffer_for_encoding = [=](AVFrame * frame, int size) {
        av_frame_unref(frame);

        frame->nb_samples = size;
        frame->channels = encoder_fmt_ctx->streams[0]->codecpar->channels;
        frame->channel_layout = encoder_fmt_ctx->streams[0]->codecpar->channel_layout;
        frame->format = encoder_fmt_ctx->streams[0]->codecpar->format;
        frame->sample_rate = encoder_fmt_ctx->streams[0]->codecpar->sample_rate;

        av_frame_get_buffer(frame, 0);
    };

    int64_t first_pts = 0;
    while(first_pts < encoder_ctx->frame_size * 256) {

        while(av_audio_fifo_size(audio_buffer) < encoder_ctx->frame_size) {
            av_packet_unref(in_packet);
            int ret = av_read_frame(decoder_fmt_ctx, in_packet);
            if (in_packet->stream_index != audio_stream_idx) {
                continue;
            }

            ret = avcodec_send_packet(decoder_ctx, in_packet);
            while(ret >= 0) {
                av_frame_unref(decoded_frame);
                ret = avcodec_receive_frame(decoder_ctx, decoded_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    LOG(ERROR) << "[RECORDING] avcodec_receive_frame()";
                    return ret;
                }

                CHECK(av_audio_fifo_realloc(audio_buffer, av_audio_fifo_size(audio_buffer) + decoded_frame->nb_samples) >= 0);

                alloc_frame_buffer_for_encoding(resampled_frame, decoded_frame->nb_samples);
                CHECK(swr_convert(swr_ctx,
                                  (uint8_t **)resampled_frame->data, decoded_frame->nb_samples,
                                  (const uint8_t **)decoded_frame->data,  decoded_frame->nb_samples) >= 0);

                CHECK(av_audio_fifo_write(audio_buffer, (void **)resampled_frame->data, resampled_frame->nb_samples) >= decoded_frame->nb_samples);
            }
        }

        while(av_audio_fifo_size(audio_buffer) >= encoder_ctx->frame_size) {
            alloc_frame_buffer_for_encoding(resampled_frame, encoder_ctx->frame_size);
            CHECK(av_audio_fifo_read(audio_buffer, (void **)resampled_frame->data, encoder_ctx->frame_size) >= encoder_ctx->frame_size);

            resampled_frame->pts = first_pts;
            first_pts += resampled_frame->nb_samples;

            int ret = avcodec_send_frame(encoder_ctx, resampled_frame);
            while(ret >= 0) {
                av_packet_unref(out_packet);
                ret = avcodec_receive_packet(encoder_ctx, out_packet);
                if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if(ret < 0) {
                    LOG(ERROR) << "[RECORDING] avcodec_receive_packet()";
                    return ret;
                }

                LOG(INFO) << fmt::format("[RECORDING] packet = {:>5d}, pts = {:>8d}, size = {:>6d}",
                                         encoder_ctx->frame_number, out_packet->pts,  out_packet->size);

                out_packet->stream_index = 0;
                if (av_interleaved_write_frame(encoder_fmt_ctx, out_packet) != 0) {
                    LOG(ERROR) << "[RECORDING] av_interleaved_write_frame()";
                    return -1;
                }
            }
        }
    }

    av_write_trailer(encoder_fmt_ctx);
    if (encoder_fmt_ctx && !(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&encoder_fmt_ctx->pb);

    avformat_close_input(&decoder_fmt_ctx);
    avformat_free_context(encoder_fmt_ctx);

    avcodec_free_context(&decoder_ctx);
    avcodec_free_context(&encoder_ctx);

    return 0;
}