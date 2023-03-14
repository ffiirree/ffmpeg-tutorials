# Linux Audio

- ALSA
- PulseAudio
- Jack
- Pipewire

## PulseAudio

List sources & sinks

```log
WARNING: Logging before InitGoogleLogging() is written to STDERR
I20230314 15:03:23.040124 62991 main.cpp:10] pulse sources:
I20230314 15:03:23.040650 62992 linux-pulse.cpp:28] PA_CONTEXT_READY
I20230314 15:03:23.040949 62992 linux-pulse.cpp:60] name: alsa_output.pci-0000_00_1f.3.iec958-stereo.monitor              , desc: Monitor of Built-in Audio Digital Stereo (IEC958)               , format: s16le , rate: 44100, channels: 2, state: SUSPENDED
I20230314 15:03:23.041002 62992 linux-pulse.cpp:60] name: alsa_input.usb-046d_081b_D16189C0-02.mono-fallback              , desc: Webcam C310 Mono                                                , format: s16le , rate: 48000, channels: 1, state: SUSPENDED
I20230314 15:03:23.041023 62992 linux-pulse.cpp:60] name: alsa_output.pci-0000_01_00.1.hdmi-stereo-extra2.monitor         , desc: Monitor of TU104 HD Audio Controller Digital Stereo (HDMI 3)    , format: s16le , rate: 44100, channels: 2, state:      IDLE
I20230314 15:03:23.041046 62991 main.cpp:13] pulse sinks:
I20230314 15:03:23.041152 62992 linux-pulse.cpp:73] name: alsa_output.pci-0000_00_1f.3.iec958-stereo                      , desc: Built-in Audio Digital Stereo (IEC958)                          , format: s16le , rate: 44100, channels: 2, state: SUSPENDED
I20230314 15:03:23.041182 62992 linux-pulse.cpp:73] name: alsa_output.pci-0000_01_00.1.hdmi-stereo-extra2                 , desc: TU104 HD Audio Controller Digital Stereo (HDMI 3)               , format: s16le , rate: 44100, channels: 2, state:   RUNNING
```
