#include "wasapi-rendering.h"
#include "audiodecoder.h"
#include "logging.h"
#include "fmt/format.h"
extern "C"{
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

int main(int argc, char* argv[])
{
    Logger::init(argv);
    CHECK(argc >= 2);

    const char * in_filename = argv[1];

    // render @{
    WasapiRender render;
    CHECK(render.open() >= 0);

    LOG(INFO) << "[WASAPI] sample_rate = " << render.sample_rate()
              << ", channels = " << render.channels()
              << ", format = " << av_get_sample_fmt_name(render.format())
              << ", channel_layout = " << render.channel_layout();
    // @}

    // decoder @{
    AudioDecoder decoder;
    CHECK(decoder.open(in_filename) >= 0);
    // @}

    // resample @{
    auto swr_ctx_ = swr_alloc_set_opts(nullptr,
                                       render.channel_layout(), render.format(), render.sample_rate(),
                                       decoder.channel_layout(), decoder.format(), decoder.sample_rate(),
                                       0, nullptr);

    CHECK(swr_ctx_ && swr_init(swr_ctx_) >= 0);

    AVAudioFifo *audio_buffer = av_audio_fifo_alloc(render.format(), render.channels(), 1);
    CHECK_NOTNULL(audio_buffer);
    defer(av_audio_fifo_free(audio_buffer));
    // @}

    decoder.run();
    render.run([&](unsigned char ** ptr, unsigned int size, unsigned long * flags)->int {
        while(av_audio_fifo_size(audio_buffer) < size && (decoder.running() || !decoder.empty())) {
            av_usleep(10'000);
        }

        if (av_audio_fifo_size(audio_buffer) < size) {
            return AVERROR_EOF;
        }

        av_audio_fifo_read(audio_buffer, (void **)ptr, size);
        return 0;
    });

    AVFrame *decoded_frame = av_frame_alloc(); defer(av_frame_free(&decoded_frame));
    char * temp_buffer = nullptr; defer(av_freep(&temp_buffer));
    unsigned int temp_buffer_size = 0;
    while(decoder.running() || !decoder.empty()) {
        if (decoder.empty()) {
            av_usleep(10'000);
            continue;
        }

        while(av_audio_fifo_size(audio_buffer) < render.sample_rate()) {
            if (decoder.produce(decoded_frame) < 0)
                break;

            auto frame_bytes = av_samples_get_buffer_size(nullptr, decoded_frame->channels, decoded_frame->nb_samples, (enum AVSampleFormat)decoded_frame->format, 0);
            av_fast_malloc(&temp_buffer, &temp_buffer_size, frame_bytes);

            CHECK(swr_convert(swr_ctx_,
                              (uint8_t **)&temp_buffer, decoded_frame->nb_samples,
                              (const uint8_t **)decoded_frame->data,  decoded_frame->nb_samples) >= 0);

            CHECK(av_audio_fifo_realloc(audio_buffer, av_audio_fifo_size(audio_buffer) + decoded_frame->nb_samples) >= 0);
            CHECK(av_audio_fifo_write(audio_buffer, (void **)&temp_buffer, decoded_frame->nb_samples) >= decoded_frame->nb_samples);
        }
    }

    render.wait(); // destroy render before audio_buffer
    return 0;
}