# Record audio on Windows

## Windows Core Audio APIs

The Core Audio APIs were introduced in Windows Vista. This new set of user-mode audio components provide client applications with improved audio capabilities. These capabilities include the following:

- Low-latency, glitch-resilient audio streaming.
- Improved reliability (many audio functions have moved from kernel-mode to user-mode).
- Improved security (processing of protected audio content takes place in a secure, lower-privilege process).
- Assignment of particular system-wide roles (console, multimedia, and communications) to individual audio devices.
- Software abstraction of the audio endpoint devices (for example, speakers, headphones, and microphones) that the user manipulates directly.

These APIs serve as the foundation for the following higher-level APIs:

- DirectSound
- DirectMusic
- Windows multimedia waveXxx and mixerXxx functions
Media Foundation

The Core Audio APIs are:

- `Multimedia Device (MMDevice) API`. Clients use this API to enumerate the audio endpoint devices in the system.
- `Windows Audio Session API (WASAPI)`. Clients use this API to create and manage audio streams to and from audio endpoint devices.
- `DeviceTopology API`. Clients use this API to directly access the topological features (for example, volume controls and multiplexers) that lie along the data paths inside hardware devices in audio adapters.
- `EndpointVolume API`. Clients use this API to directly access the volume controls on audio endpoint devices. This API is primarily used by applications that manage exclusive-mode audio streams.

![Core Audio APIs](/11_wasapi_capture/core_audio.jpg)

**A client of WASAPI passes data to an endpoint device through an `endpoint buffer`**. System software and hardware components manage the movement of data from the endpoint buffer to the endpoint device in a manner that is largely transparent to the client. Furthermore, for an endpoint device that plugs into an audio adapter with jack-presence detection, the client can create an endpoint buffer only for an endpoint device that is physically present. For more information about jack-presence detection, see Audio Endpoint Devices.

The preceding diagram shows two types of endpoint buffer. If a client of WASAPI opens a stream in `shared mode`, then the client writes audio data to the endpoint buffer and the Windows audio engine reads the data from the buffer. *In this mode, the client shares the audio hardware with other applications running in other processes.* The audio engine mixes the streams from these applications and plays the resulting mix through the hardware. The audio engine is a user-mode system component (Audiodg.dll) that performs all of its stream-processing operations in software. In contrast, if a client opens a stream in `exclusive mode`, the client has exclusive access to the audio hardware. Typically, only a small number of "pro audio" or RTC applications require exclusive mode. Although the diagram shows both the shared-mode and exclusive-mode streams, only one of these two streams (and its corresponding endpoint buffer) exists, depending on whether the client opens the stream in shared mode or in exclusive mode.

In exclusive mode, the client can choose to open the stream in any audio format that the endpoint device supports. In shared mode, the client must open the stream in the mix format that is currently in use by the audio engine (or a format that is similar to the mix format). The audio engine's input streams and the output mix from the engine are all in this format.

In Windows 7, a new feature called `low-latence mode` has been added for streams in share mode. In this mode, the audio engine runs in pull mode, in which there a significant reduction in latency. This is very useful for communication applications that require low audio stream latency for faster streaming.

Applications that manage low-latency audio streams can use the Multimedia Class Scheduler Service (MMCSS) in Windows Vista to increase the priority of application threads that access endpoint buffers. MMCSS enables audio applications to run at high priority without denying CPU resources to lower-priority applications. MMCSS assigns a priority to a thread based on its task name. For example, Windows Vista supports the task names "Audio" and "Pro Audio" for threads that manage audio streams. By default, the priority of a "Pro Audio" thread is higher than that of an "Audio" thread. For more information about MMCSS, see the Windows SDK documentation.

**The core audio APIs support both PCM and non-PCM stream formats. However, the audio engine can mix only PCM streams.** Thus, only exclusive-mode streams can have non-PCM formats. For more information, see Device Formats.

The audio engine runs in its own protected process, which is separate from the process that the application runs in. To support a shared-mode stream, the Windows audio service (the box labeled "Audio Service" in the preceding diagram) allocates a cross-process endpoint buffer that is accessible to both the application and the audio engine. For exclusive mode, the endpoint buffer resides in memory that is accessible to both the application and the audio hardware.

The Windows audio service is the module that implements Windows audio policy. Audio policy is a set of internal rules that the system applies to the interactions between audio streams from multiple applications that share and compete for use of the same audio hardware. The Windows audio service implements audio policy by setting the control parameters for the audio engine. The duties of the audio service include:

Keeping track of the audio devices that the user adds to or removes from the system.
Monitoring the roles that are assigned to the audio devices in the system.
Managing the audio streams from groups of tasks that produce similar classes of audio content (console, multimedia, and communications).
Controlling the volume level of the combined output stream ("submix") for each of the various types of audio content.
Informing the audio engine about the processing elements in the data paths for the audio streams.
In some versions of Windows, the Windows audio service is disabled by default and must be explicitly turned on before the system can play audio.


## MMDevice API

The Windows Multimedia Device (MMDevice) API enables audio clients to discover audio endpoint devices, determine their capabilities, and create driver instances for those devices.


## Device Roles


| ERole constant  | Device role                              | Rendering examples             | Capture examples                   |
| --------------- | ---------------------------------------- | ------------------------------ | ---------------------------------- |
| eConsole        | Interaction with the computer            | Games and system notifications | Voice commands                     |
| eCommunications | Voice communications with another person | Chat and VoIP                  | Chat and VoIP                      |
| eMultimedia     | Playing or recording audio content       | Music and movies               | Narration and live music recording |

## References

- [About WASAPI](https://docs.microsoft.com/en-us/windows/win32/coreaudio/wasapi)
- [WASAPI : Capturing a Stream](https://learn.microsoft.com/en-us/windows/win32/coreaudio/capturing-a-stream)
- [WASAPI : Loopback Recording](hhttps://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording)