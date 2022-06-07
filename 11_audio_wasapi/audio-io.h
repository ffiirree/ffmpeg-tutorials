#ifndef AUDIO_IO_H
#define AUDIO_IO_H

#include "utils.h"

#ifdef _WIN32
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

class MyAudioSink {
public:
    int SetFormat(WAVEFORMATEX * fmt)
    {
        fmt_ = *fmt;

        // convert to ffmpeg format

        return 0;
    }

    int CopyData(unsigned char * data, uint32_t nb_frames, int* is_done)
    {
        // silence
        if(data == nullptr) {
            //
        }
        else {
            // process data
        }

        return 0;
    }

    WAVEFORMATEX fmt_;
};

int record_audio_stream(MyAudioSink *);

#endif

int enum_audio_endpoints();
int default_audio_endpoint();

#endif //!AUDIO_IO_H