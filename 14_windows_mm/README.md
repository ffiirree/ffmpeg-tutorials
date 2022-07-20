# Windows 多媒体

关于Windows上**录屏**/**录音**的实现有很多方法，这些方法主要分为两大类: `Audio and Video`和`Graphics and gaming`。两类方法在Windows的发展过程中都经过了多次的迭代，因此具体的实现方法有很多种，这些方法有适用于不同的Windows版本和应用场景。

![Multimedia](/14_windows_mm/win_mm_dep.png)

## Audio and Video

Provides guidance about using audio and video features provided by Windows.

### Core Audio APIs

A low-level API for audio capture and audio rendering, which can be used to achieve minimum latency or to implement features that might not be entirely supported by higher-level media APIs.

The core audio APIs provide the means for audio applications to access audio endpoint devices such as headphones and microphones. The core audio APIs serve as the foundation for higher-level audio APIs such as Microsoft DirectSound and the Windows multimedia waveXxx functions. Most applications communicate with the higher-level APIs, but some applications with special requirements might need to communicate directly with the core audio APIs.

Starting with Windows 7, the existing APIs have been improved and new APIs have been added to support new scenarios. The stream and session management APIs have been improved so that the application can now enumerate and get extended control over the audio session. By using the new APIs, the application can implement a custom stream attenuation experience. New device-related APIs provide access to the driver properties of the endpoint devices.

### DirectShow

The Microsoft DirectShow application programming interface (API) is a media-streaming architecture for Microsoft Windows. Using DirectShow, your applications can perform high-quality video and audio playback or capture.

Microsoft® DirectShow® is an architecture for streaming media on the Microsoft Windows® platform. DirectShow provides for high-quality capture and playback of multimedia streams. It supports a wide variety of formats, including Advanced Systems Format (ASF), Motion Picture Experts Group (MPEG), Audio-Video Interleaved (AVI), MPEG Audio Layer-3 (MP3), and WAV sound files. It supports capture from digital and analog devices based on the Windows Driver Model (WDM) or Video for Windows. It automatically detects and uses video and audio acceleration hardware when available, but also supports systems without acceleration hardware.

DirectShow is based on the Component Object Model (COM). To write a DirectShow application or component, you must understand COM client programming. For most applications, you do not need to implement your own COM objects. DirectShow provides the components you need. If you want to extend DirectShow by writing your own components, however, you must implement them as COM objects.

DirectShow is designed for C++. Microsoft does not provide a managed API for DirectShow.

DirectShow simplifies media playback, format conversion, and capture tasks. At the same time, it provides access to the underlying stream control architecture for applications that require custom solutions. You can also create your own DirectShow components to support new formats or custom effects.

### Microsoft Media Foundation

## Graphics and gaming

Provides information about graphics and gaming features of Windows, including DirectX and digital imaging.

![Windows Graphics Architecture](/14_windows_mm/win_graphics_arch.png)

### Windows GDI

### DirectX Graphics and Gaming

Microsoft DirectX graphics provides a set of APIs that you can use to create games and other high-performance multimedia applications. DirectX graphics includes support for high-performance 2-D and 3-D graphics.

For 3-D graphics, use the Microsoft Direct3D 11 API. Even if you have Microsoft Direct3D 9-level or Microsoft Direct3D 10-level hardware, you can use the Direct3D 11 API and target a feature level 9_x or feature level 10_x device. For info about how to develop 3-D graphics with DirectX, see Create 3D graphics with DirectX.

For 2-D graphics and text, use Direct2D and DirectWrite rather than Windows Graphics Device Interface (GDI).

To compose bitmaps that Direct3D 11 or Direct2D populated, use DirectComposition.

### OBS Studio 画面捕获实现

OBS Studio 在Windows上提供了三种捕获模式：

- `window capture`: 捕获单个应用窗口(非全屏游戏窗口)
- `game capture`：捕获游戏窗口
- `monitor capture`：捕获整个显示器(桌面)
  
推荐使用`window capture`，如果捕获游戏则使用`game capture`，这两种模式较为高效(`monitor capture`只在Windows 8上非常高效(不知道OBS Studio写这篇文章的时候win10/11出了没)，只有`game capture`能够捕获**全屏**模式下的游戏画面)。

## References

- [Desktop app technologies](https://docs.microsoft.com/en-gb/windows/win32/desktop-app-technologies)
- [Overview of the Windows Graphics Architecture](https://docs.microsoft.com/en-us/windows/win32/learnwin32/overview-of-the-windows-graphics-architecture)
- [Windows多媒体开发框架介绍](https://blog.csdn.net/jay103/article/details/86665419)
- [[OBS-Studio] Which capture method?](https://jp9000.github.io/OBS/general/whatcapture.html)
- [The Desktop Window Manager(DWM)](https://docs.microsoft.com/en-us/windows/win32/learnwin32/the-desktop-window-manager)
- [Audio and Video](https://docs.microsoft.com/en-gb/windows/win32/audio-and-video)
- [Graphics and gaming](https://docs.microsoft.com/en-gb/windows/win32/graphics-and-multimedia)
- [DirectShow](https://docs.microsoft.com/en-us/windows/win32/directshow/directshow)
- [Windows GDI](https://docs.microsoft.com/en-us/windows/win32/gdi/windows-gdi)
- [obs-studio/win-capture](https://github.com/obsproject/obs-studio/tree/master/plugins/win-capture)
- [obs-studio/win-dshow](https://github.com/obsproject/obs-studio/tree/master/plugins/win-dshow)
- [obs-studio/win-wasapi](https://github.com/obsproject/obs-studio/tree/master/plugins/win-wasapi)