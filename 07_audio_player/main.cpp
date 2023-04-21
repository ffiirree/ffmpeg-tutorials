#include "logging.h"
#include "audiodecoder.h"
#include "audioplayer.h"
extern "C" {
#include <libavutil/audio_fifo.h>
#include <libswresample/swresample.h>
}

int main(int argc, char *argv[])
{
    Logger::init(argv[0]);
    CHECK(argc >= 2) << "audio_player <input>";

    AudioDecoder decoder;
    CHECK(decoder.open(argv[1]) >= 0);

    AudioPlayer player;
    CHECK(player.open(48000, 2, av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * 8) >= 0);

    auto swr_ctx_ = swr_alloc_set_opts(nullptr,
                                       av_get_default_channel_layout(2), AV_SAMPLE_FMT_S16, 48000,
                                       decoder.channel_layout(), decoder.format(), decoder.sample_rate(),
                                       0, nullptr);

    CHECK(swr_ctx_ && swr_init(swr_ctx_) >= 0);

    AVFrame *decoded_frame = av_frame_alloc(); defer(av_frame_free(&decoded_frame));
    char * temp_buffer = nullptr; defer(av_freep(&temp_buffer));
    unsigned int temp_buffer_size = 0;

    decoder.run();
    while(!decoder.empty() || decoder.running()) {
        if (decoder.empty()) {
            av_usleep(15'000);
            continue;
        }

        if (decoder.produce(decoded_frame) >= 0) {
            int64_t frame_bytes = decoded_frame->nb_samples * 2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
            while (player.buffer_free_size() < frame_bytes) {
                av_usleep(15'000);
            }

            av_fast_malloc(&temp_buffer, &temp_buffer_size, frame_bytes);
            CHECK(swr_convert(swr_ctx_,
                              (uint8_t **)&temp_buffer, decoded_frame->nb_samples,
                              (const uint8_t**)decoded_frame->data, decoded_frame->nb_samples) >= 0);

            player.write((const char *)temp_buffer, frame_bytes);
        }
    }

    return 0;
}
