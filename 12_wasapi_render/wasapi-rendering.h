#ifndef WASAPI_RENDERING_H
#define WASAPI_RENDERING_H

#include <Windows.h>
#include <mmdeviceapi.h>
#include <propsys.h>
#include <functiondiscoverykeys.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <Audioclient.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}

//-----------------------------------------------------------
// Play an audio stream on the default audio rendering
// device. The PlayAudioStream function allocates a shared
// buffer big enough to hold one second of PCM audio data.
// The function uses this buffer to stream data to the
// rendering device. The inner loop runs every 1/2 second.
//-----------------------------------------------------------

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { return -1; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

class WasapiRender {
public:
    WasapiRender() = default;
    WasapiRender(const WasapiRender&) = delete;
    WasapiRender& operator=(const WasapiRender&) = delete;
    ~WasapiRender() { destroy(); }

    int open();

    int run(std::function<int(unsigned char **, unsigned int, unsigned long*)> cb)
    {
        if (running_ || !ready_) {
            return -1;
        }
        running_ = true;
        callback_ = cb;
        thread_ = std::thread([this]() { render_f(); });
    }

    int sample_rate() const { return sample_rate_; }
    int channels() const { return channels_; }
    enum AVSampleFormat format() const { return sample_fmt_; }
    uint64_t channel_layout() const { return channel_layout_; }
    AVRational time_base() const { return time_base_; }

    void wait() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

private:
    int render_f();
    int destroy();

    std::function<int(unsigned char **, unsigned int, unsigned long*)> callback_;

    std::atomic<bool> running_{ false };
    std::atomic<bool> ready_{ false };

    std::thread thread_;

    // audio params @{
    int sample_rate_ { 44100 };
    int channels_{ 2 };
    int bit_rate_ { 0 };
    enum AVSampleFormat sample_fmt_{ AV_SAMPLE_FMT_FLT };
    uint64_t channel_layout_{ 0 };
    AVRational time_base_{ 0, AV_TIME_BASE };
    // @}

    // WASAPI @{
    IAudioClient* audio_client_{ nullptr };
    IAudioRenderClient* render_client_{ nullptr };
    UINT32 buffer_nb_frames_{ 0 };
    // @}
};
HRESULT PlayAudioStream(std::function<int(unsigned char *, uint32_t, uint64_t *)> callback);
#endif //!WASAPI_RENDERING_H