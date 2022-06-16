#include "wasapi-rendering.h"
#include "utils.h"

uint64_t to_ffmpeg_channel_layout(DWORD layout, int channels)
{
    switch (layout) {
        case KSAUDIO_SPEAKER_MONO: return AV_CH_LAYOUT_MONO;
        case KSAUDIO_SPEAKER_STEREO: return AV_CH_LAYOUT_STEREO;
        case KSAUDIO_SPEAKER_QUAD: return AV_CH_LAYOUT_QUAD;
        case KSAUDIO_SPEAKER_2POINT1: return AV_CH_LAYOUT_SURROUND;
        case KSAUDIO_SPEAKER_SURROUND: return AV_CH_LAYOUT_4POINT0;
        default: return av_get_default_channel_layout(channels);
    }
}

int WasapiRender::open()
{
    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    IMMDeviceEnumerator *pEnumerator = nullptr;
    IMMDevice *pDevice = nullptr;
    WAVEFORMATEX *pwfx = nullptr;

    EXIT_ON_ERROR(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

    EXIT_ON_ERROR(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                   CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                   (void**)&pEnumerator));
    defer(SAFE_RELEASE(pEnumerator));

    EXIT_ON_ERROR(pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice));
    defer(SAFE_RELEASE(pDevice));

    EXIT_ON_ERROR(pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                                    nullptr, (void**)&audio_client_));

    EXIT_ON_ERROR(audio_client_->GetMixFormat(&pwfx));
    defer(CoTaskMemFree(pwfx));

    EXIT_ON_ERROR(audio_client_->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                            0,
                                            hnsRequestedDuration,
                                            0,
                                            pwfx,
                                            nullptr));

    DWORD layout = 0;
    if(pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        auto pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
        layout = pEx->dwChannelMask;
    }

    sample_rate_ = pwfx->nSamplesPerSec;
    channels_ = pwfx->nChannels;
    bit_rate_ = pwfx->nAvgBytesPerSec;
    sample_fmt_ = AV_SAMPLE_FMT_FLT;
    channel_layout_ = to_ffmpeg_channel_layout(layout, channels_);

    // Get the actual size of the allocated buffer.
    EXIT_ON_ERROR(audio_client_->GetBufferSize(&buffer_nb_frames_))

    EXIT_ON_ERROR(audio_client_->GetService(__uuidof(IAudioRenderClient),(void**)&render_client_));

    ready_ = true;
    return 0;
}

int WasapiRender::render_f()
{
    REFERENCE_TIME hnsActualDuration;
    UINT32 numFramesAvailable;
    UINT32 numFramesPadding;
    BYTE *pData;
    DWORD flags = 0;

    // Grab the entire buffer for the initial fill operation.
    EXIT_ON_ERROR(render_client_->GetBuffer(buffer_nb_frames_, &pData));

    // Load the initial data into the shared buffer.
    EXIT_ON_ERROR(callback_(&pData, buffer_nb_frames_, &flags))

    EXIT_ON_ERROR(render_client_->ReleaseBuffer(buffer_nb_frames_, flags));

    // Calculate the actual duration of the allocated buffer.
    hnsActualDuration = (double)REFTIMES_PER_SEC * buffer_nb_frames_ / sample_rate_;

    EXIT_ON_ERROR(audio_client_->Start());  // Start playing.

    // Each loop fills about half of the shared buffer.
    while (flags != AUDCLNT_BUFFERFLAGS_SILENT && running_)
    {
        // Sleep for half the buffer duration.
        Sleep((DWORD)(hnsActualDuration/REFTIMES_PER_MILLISEC/2));

        // See how much buffer space is available.
        EXIT_ON_ERROR(audio_client_->GetCurrentPadding(&numFramesPadding));

        numFramesAvailable = buffer_nb_frames_ - numFramesPadding;

        // Grab all the available space in the shared buffer.
        EXIT_ON_ERROR(render_client_->GetBuffer(numFramesAvailable, &pData));

        // Get next 1/2-second of data from the audio source.
        if (callback_(&pData, numFramesAvailable, &flags)) {
            running_ = false;
        }

        EXIT_ON_ERROR(render_client_->ReleaseBuffer(numFramesAvailable, flags));
    }

    // Wait for last data in buffer to play before stopping.
    Sleep((DWORD)(hnsActualDuration/REFTIMES_PER_MILLISEC/2));

    return audio_client_->Stop();  // Stop playing.
}

int WasapiRender::destroy()
{
    running_ = false;

    wait();

    if(audio_client_) {
        audio_client_->Stop();
        SAFE_RELEASE(audio_client_)
    }

    SAFE_RELEASE(render_client_)

    CoUninitialize();

    return 0;
}