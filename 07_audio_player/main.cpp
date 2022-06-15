#include "logging.h"
#include "audiodecoder.h"
#include "audioplayer.h"
extern "C" {
#include <libavutil/audio_fifo.h>
#include <libswresample/swresample.h>
}

int main(int argc, char *argv[])
{
    Logger::init(argv);

    if (argc < 2) {
        LOG(ERROR) << "player <input>";
        return -1;
    }

    AudioDecoder decoder;
    CHECK(decoder.open(argv[1]) >= 0);

    AudioPlayer player;
    CHECK(player.open(48000, 2, av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * 8) >= 0);

    auto swr_ctx_ = swr_alloc_set_opts(nullptr,
                                       av_get_default_channel_layout(2), AV_SAMPLE_FMT_S16, 48000,
                                       decoder.channel_layout(), decoder.format(), decoder.sample_rate(),
                                       0, nullptr);

    CHECK(swr_ctx_ && swr_init(swr_ctx_) >= 0);

    decoder.run();

    AVAudioFifo *fifo_buffer = av_audio_fifo_alloc(AV_SAMPLE_FMT_S16, 2, 1);
    CHECK_NOTNULL(fifo_buffer);
    defer(av_audio_fifo_free(fifo_buffer));

    AVFrame *decoded_frame = av_frame_alloc(); defer(av_frame_free(&decoded_frame));

    char * temp_buffer = nullptr; defer(av_freep(&temp_buffer));
    unsigned int temp_buffer_size = 0;

    while(!decoder.empty() || decoder.running()) {
        int period_samples = player.period_size() / (2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16));

        if ((decoder.empty() && av_audio_fifo_size(fifo_buffer) < period_samples) ||
            player.buffer_free_size() < player.period_size()) {
            av_usleep(10000);
            continue;
        }

        while(av_audio_fifo_size(fifo_buffer) < period_samples && decoder.produce(decoded_frame) >= 0) {
            av_fast_malloc(&temp_buffer, &temp_buffer_size, decoded_frame->nb_samples * (2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16)));
            CHECK(swr_convert(swr_ctx_,
                              (uint8_t **)&temp_buffer, decoded_frame->nb_samples,
                              (const uint8_t**)decoded_frame->data, decoded_frame->nb_samples) >= 0);

            CHECK(av_audio_fifo_realloc(fifo_buffer, av_audio_fifo_size(fifo_buffer) + decoded_frame->nb_samples) >= 0);
            CHECK(av_audio_fifo_write(fifo_buffer, (void **)&temp_buffer, decoded_frame->nb_samples) >= decoded_frame->nb_samples);
        }

        while(av_audio_fifo_size(fifo_buffer) >= period_samples || (!decoder.running() && decoder.empty() && av_audio_fifo_size(fifo_buffer) > 0)) {
            auto read_samples = std::min(period_samples, av_audio_fifo_size(fifo_buffer));
            av_fast_malloc(&temp_buffer, &temp_buffer_size, read_samples * (2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16)));

            CHECK(av_audio_fifo_read(fifo_buffer, (void **)&temp_buffer, read_samples) >= read_samples);
            player.write((const char *)temp_buffer, read_samples * 2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16));
        }
    }

    return 0;
}
