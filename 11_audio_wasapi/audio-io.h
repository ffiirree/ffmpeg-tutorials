#ifndef AUDIO_IO_H
#define AUDIO_IO_H

#include "utils.h"
#include "ringvector.h"

#define RETURN_ON_ERROR(hres)  if (FAILED(hres)) { return -1; }
#define SAFE_RELEASE(punk)  if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; }

// The Windows Multimedia Device (MMDevice) API enables audio clients to
// discover audio endpoint devices, determine their capabilities,
// and create driver instances for those devices.
// https://docs.microsoft.com/en-us/windows/win32/coreaudio/mmdevice-api

#include <Windows.h>
#include <mmdeviceapi.h>
#include <propsys.h>
#include <functiondiscoverykeys.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <Audioclient.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}

enum class DeviceType {
    DEVICE_SPEAKER,
    DEVICE_MICROPHONE
};

class WasapiCapturer {
public:

public:
    WasapiCapturer() = default;
    WasapiCapturer(const WasapiCapturer&) = delete;
    WasapiCapturer& operator=(const WasapiCapturer&) = delete;
    ~WasapiCapturer() { reset(); }

    int open(DeviceType);
    int reset() { return destroy();}
    void stop() { running_ = false; }

    int produce(AVFrame *frame);

    int run();
    bool empty() const { return buffer_.empty(); }
    bool ready() const { return ready_; }
    bool running() const { return running_; }

    int64_t start_time() const { return start_time_; }

    int sample_rate() const { return sample_rate_; }
    int channels() const { return channels_; }
    enum AVSampleFormat format() const { return sample_fmt_; }
    uint64_t channel_layout() const { return channel_layout_; }
    AVRational time_base() const { return time_base_; }
private:
    int destroy();
    int run_f();
    uint64_t to_ffmpeg_channel_layout(DWORD layout, int channels);

    std::atomic<bool> ready_{ false };
    std::atomic<bool> running_{ false };
    std::thread thread_;
    std::mutex mtx_;

    int64_t start_time_{ AV_NOPTS_VALUE };

    RingVector<AVFrame*, 16> buffer_{
            []() { return av_frame_alloc(); },
            [](AVFrame** frame) { av_frame_free(frame); }
    };

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
    IAudioCaptureClient* capture_client_{ nullptr };
    UINT32 buffer_nb_frames_{ 0 };
    // @}
};

int enum_audio_endpoints();
int default_audio_endpoint();

#endif //!AUDIO_IO_H