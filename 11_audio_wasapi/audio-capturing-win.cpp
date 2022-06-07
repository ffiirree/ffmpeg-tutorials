#include "audio-io.h"

#ifdef _WIN32

#include <Audioclient.h>

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

// https://docs.microsoft.com/en-us/windows/win32/coreaudio/capturing-a-stream
int record_audio_stream(MyAudioSink *pMySink)
{
    HRESULT hr;
    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    REFERENCE_TIME hnsActualDuration;
    UINT32 bufferFrameCount;
    UINT32 numFramesAvailable;
    IMMDeviceEnumerator *pEnumerator = nullptr;
    IMMDevice *pDevice = nullptr;
    IAudioClient *pAudioClient = nullptr;
    IAudioCaptureClient *pCaptureClient = nullptr;
    WAVEFORMATEX *pwfx = nullptr;
    UINT32 packetLength = 0;
    BOOL bDone = FALSE;
    BYTE *pData;
    DWORD flags;

    RETURN_ON_ERROR(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
    defer(CoUninitialize());

    RETURN_ON_ERROR(CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            (void**)&pEnumerator
    ));
    defer(SAFE_RELEASE(pEnumerator));

    RETURN_ON_ERROR(pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDevice));
    defer(SAFE_RELEASE(pDevice));

    RETURN_ON_ERROR(pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void**)&pAudioClient));
    defer(SAFE_RELEASE(pAudioClient));

    RETURN_ON_ERROR(pAudioClient->GetMixFormat(&pwfx));
    defer(CoTaskMemFree(pwfx));

    RETURN_ON_ERROR(pAudioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            0,
            hnsRequestedDuration,
            0,
            pwfx,
            nullptr
    ));

    // Get the size of the allocated buffer.
    RETURN_ON_ERROR(pAudioClient->GetBufferSize(&bufferFrameCount));

    RETURN_ON_ERROR(pAudioClient->GetService(
            IID_IAudioCaptureClient,
            (void**)&pCaptureClient
    ));
    defer(SAFE_RELEASE(pCaptureClient));

    // Notify the audio sink which format to use.
    RETURN_ON_ERROR(pMySink->SetFormat(pwfx));

    // Calculate the actual duration of the allocated buffer.
    hnsActualDuration = (double)REFTIMES_PER_SEC * bufferFrameCount / pwfx->nSamplesPerSec;

    RETURN_ON_ERROR(pAudioClient->Start());  // Start recording.

    // Each loop fills about half of the shared buffer.
    while (bDone == FALSE)
    {
        // Sleep for half the buffer duration.
        Sleep(hnsActualDuration / REFTIMES_PER_MILLISEC / 2);

        RETURN_ON_ERROR(pCaptureClient->GetNextPacketSize(&packetLength));

        while (packetLength != 0)
        {
            // Get the available data in the shared buffer.
            RETURN_ON_ERROR(pCaptureClient->GetBuffer(
                    &pData,
                    &numFramesAvailable,
                    &flags,
                    nullptr,
                    nullptr
            ));

            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                pData = nullptr;  // Tell CopyData to write silence.
            }

            // Copy the available capture data to the audio sink.
            RETURN_ON_ERROR(pMySink->CopyData(pData, numFramesAvailable, &bDone));

            RETURN_ON_ERROR(pCaptureClient->ReleaseBuffer(numFramesAvailable));

            RETURN_ON_ERROR(pCaptureClient->GetNextPacketSize(&packetLength));
        }
    }

    RETURN_ON_ERROR(pAudioClient->Stop());  // Stop recording.

    return 0;
}

#endif