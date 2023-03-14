#include "audio-io.h"
#include "logging.h"
#include "fmt/format.h"
extern "C"{
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

int main(int argc, char* argv[])
{
    Logger::init(argv);

    const char * out_filename = argc >= 2 ? argv[1] : "out.mp4";

    // devices @{
    auto output_devices = enum_audio_endpoints(false);
    for (auto& [name, id] : output_devices) {
        LOG(INFO) << "[OUTPUT] name = '" << name << "', id = '" << id << "'";
    }

    auto input_devices = enum_audio_endpoints(true);
    for (auto& [name, id] : input_devices) {
        LOG(INFO) << "[ INPUT] name = '" << name << "', id = '" << id << "'";
    }

    auto default_out_device = default_audio_endpoint(false);
    if (default_out_device.has_value()) {
        LOG(INFO) << "[OUTPUT] name = '" << default_out_device->first << "', id = '" << default_out_device->second << "' (default)";
    }

    auto default_in_device = default_audio_endpoint(true);
    if (default_out_device.has_value()) {
        LOG(INFO) << "[ INPUT] name = '" << default_in_device->first << "', id = '" << default_in_device->second << "' (default)";
    }
    // @}

    // INPUT @{@
    std::unique_ptr<WasapiCapturer> decoder = std::make_unique<WasapiCapturer>();
    if (decoder->open(DeviceType::DEVICE_SPEAKER) < 0) {
        LOG(INFO) << "open input failed";
        return -1;
    }
    // }

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
    encoder_ctx->sample_rate = decoder->sample_rate();
    encoder_ctx->channels = decoder->channels();
    encoder_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    encoder_ctx->channel_layout = av_get_default_channel_layout(decoder->channels());

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
                                             encoder_ctx->channel_layout,
                                             encoder_ctx->sample_fmt,
                                             encoder_ctx->sample_rate,
                                             decoder->channel_layout(),
                                             decoder->format(),
                                             decoder->sample_rate(),
                                             0, nullptr);
    CHECK_NOTNULL(swr_ctx);
    CHECK(swr_init(swr_ctx) >= 0);
    defer(swr_free(&swr_ctx));

    av_dump_format(encoder_fmt_ctx, 0, out_filename, 1);
    LOG(INFO) << fmt::format("[OUTPUT] codec_name = {}, sample_rate = {}Hz, channels = {}, sample_fmt = {}, channel_layout = {}, frame_size = {}, tbc = {}/{}, tbn = {}/{}",
                             encoder->name, encoder_ctx->sample_rate, encoder_ctx->channels,
                             av_get_sample_fmt_name(encoder_ctx->sample_fmt),
                             encoder_ctx->channel_layout,
                             encoder_fmt_ctx->streams[0]->codecpar->frame_size,
                             encoder_ctx->time_base.num, encoder_ctx->time_base.den,
                             encoder_fmt_ctx->streams[0]->time_base.num, encoder_fmt_ctx->streams[0]->time_base.den);
    // @}

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

    if(decoder->run() < 0) {
        LOG(INFO) << "failed to run";
        return -1;
    }

    int64_t first_pts = 0;
    while(first_pts < encoder_ctx->frame_size * 512) {
        if(decoder->empty() && av_audio_fifo_size(audio_buffer) < encoder_ctx->frame_size) {
            av_usleep(10000);
            continue;
        }

        while(av_audio_fifo_size(audio_buffer) < encoder_ctx->frame_size) {
            if (decoder->empty()) {
                break;
            }

            av_frame_unref(decoded_frame);
            int ret = 0;
            while(ret >=0) {
                ret = decoder->produce(decoded_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    LOG(ERROR) << "[ENCODING] produce()";
                    return ret;
                }

                LOG(INFO) << "[ARRIVED] samples = " << decoded_frame->nb_samples << ",  pts = " << decoded_frame->pts;

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
                    LOG(ERROR) << "[ENCODING] avcodec_receive_packet()";
                    return ret;
                }

                LOG(INFO) << fmt::format("[ENCODING] packet = {:>5d}, pts = {:>8d}, size = {:>6d}",
                                         encoder_ctx->frame_number, out_packet->pts,  out_packet->size);

                out_packet->stream_index = 0;
                if (av_interleaved_write_frame(encoder_fmt_ctx, out_packet) != 0) {
                    LOG(ERROR) << "[ENCODING] av_interleaved_write_frame()";
                    return -1;
                }
            }
        }
    }

    av_write_trailer(encoder_fmt_ctx);
    if (encoder_fmt_ctx && !(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&encoder_fmt_ctx->pb);

    avformat_free_context(encoder_fmt_ctx);

    avcodec_free_context(&encoder_ctx);

    return 0;
}