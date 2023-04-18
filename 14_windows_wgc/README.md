# Windows Graphics

## Windows 屏幕采集方法

Windows 上的屏幕采集技术是伴随着其图形图像技术的演进而进行迭代的，不同的采集方法使用了不同的技术，由于Windows优秀的兼容性，
老的采集技术依旧可以使用，只是相对于最新的技术占用更多的资源，目前至少有以下几种采集方法：

- `GDI`: 兼容各版本的Windows，占用大量CPU资源，性能差，鼠标需要单采，无法实现过滤制定窗口，`FFmpeg`的实现为`gdigrab`;
- `DXGI`: 即`Desktop Duplication API`, Windows 8 及后续版本，性能好,`FFmpeg 6.0`的实现为`ddagrab`;
- `Magnification`: 能实现放大缩小颜色转换等操作，能过滤窗口;
- `Window Graphics Capture`: Windows 10 1803 及后续版本；`D3D11`和`DXGI`使用GPU加速，占用资源更少。

> 像 obs-studio 则实现了`GDI + DDA + WGC`，运行时根据不同的系统版本和硬件条件进行采集技术的选择

其中，`GDI`和`DDA`的实现均需要**自绘鼠标**，而且`FFmpeg`中的`gdigrab`录屏时会有明显的鼠标闪烁问题，这个问题至今无法解决。

## 技术演化

关于Windows上**录屏**/**录音**的实现有很多方法，这些方法主要分为两大类: `Audio and Video`和`Graphics and gaming`。两类方法在Windows的发展过程中都经过了多次的迭代，因此具体的实现方法有很多种，这些方法有适用于不同的Windows版本和应用场景。

![Multimedia](/images/win_mm_dep.png)

## Audio and Video

Provides guidance about using audio and video features provided by Windows.

### Core Audio APIs

A low-level API for audio capture and audio rendering, which can be used to achieve minimum latency or to implement features that might not be entirely supported by higher-level media APIs.

The core audio APIs provide the means for audio applications to access audio endpoint devices such as headphones and microphones. The core audio APIs serve as the foundation for higher-level audio APIs such as Microsoft DirectSound and the Windows multimedia waveXxx functions. Most applications communicate with the higher-level APIs, but some applications with special requirements might need to communicate directly with the core audio APIs.

Starting with Windows 7, the existing APIs have been improved and new APIs have been added to support new scenarios. The stream and session management APIs have been improved so that the application can now enumerate and get extended control over the audio session. By using the new APIs, the application can implement a custom stream attenuation experience. New device-related APIs provide access to the driver properties of the endpoint devices.

### DirectShow

The Microsoft DirectShow application programming interface (API) is a media-streaming architecture for Microsoft Windows. Using DirectShow, your applications can perform high-quality video and audio playback or capture.

Microsoft DirectShow is an architecture for streaming media on the Microsoft Windows platform. DirectShow provides for high-quality capture and playback of multimedia streams. It supports a wide variety of formats, including Advanced Systems Format (ASF), Motion Picture Experts Group (MPEG), Audio-Video Interleaved (AVI), MPEG Audio Layer-3 (MP3), and WAV sound files. It supports capture from digital and analog devices based on the Windows Driver Model (WDM) or Video for Windows. It automatically detects and uses video and audio acceleration hardware when available, but also supports systems without acceleration hardware.

DirectShow is based on the Component Object Model (COM). To write a DirectShow application or component, you must understand COM client programming. For most applications, you do not need to implement your own COM objects. DirectShow provides the components you need. If you want to extend DirectShow by writing your own components, however, you must implement them as COM objects.

DirectShow is designed for C++. Microsoft does not provide a managed API for DirectShow.

DirectShow simplifies media playback, format conversion, and capture tasks. At the same time, it provides access to the underlying stream control architecture for applications that require custom solutions. You can also create your own DirectShow components to support new formats or custom effects.

![DirectShow](/images/directshow_arch.png)

### Microsoft Media Foundation

Microsoft Media Foundation enables the development of applications and components for using digital media on Windows Vista and later.

Media Foundation is the next generation multimedia platform for Windows that enables developers, consumers, and content providers to embrace the new wave of premium content with enhanced robustness, unparalleled quality, and seamless interoperability.

## Windows Graphics

Windows provides several C++/COM APIs for graphics. These APIs are shown in the following diagram.

![Windows Graphics Architecture](/images/win_graphics_arch.png)

- `Graphics Device Interface (GDI)` is the original graphics interface for Windows. GDI was first written for 16-bit Windows and then updated for 32-bit and 64-bit Windows.
- `GDI+` was introduced in Windows XP as a successor to GDI. The GDI+ library is accessed through a set of C++ classes that wrap flat C functions. The .NET Framework also provides a managed version of GDI+ in the System.Drawing namespace.
- `Direct3D` supports 3-D graphics.
- `Direct2D` is a modern API for 2-D graphics, the successor to both GDI and GDI+.
- `DirectWrite` is a text layout and rasterization engine. You can use either GDI or Direct2D to draw the rasterized text.
- `DirectX Graphics Infrastructure (DXGI)` performs low-level tasks, such as presenting frames for output. Most applications do not use DXGI directly. Rather, it serves as an intermediate layer between the graphics driver and Direct3D.

Direct2D is the focus of this module. While both GDI and GDI+ continue to be supported in Windows, **Direct2D and DirectWrite are recommended for new programs**. In some cases, a mix of technologies might be more practical. For these situations, Direct2D and DirectWrite are designed to interoperate with GDI.

While GDI supports hardware acceleration for certain operations, many GDI operations are bound to the CPU. Direct2D is layered on top of Direct3D, and takes full advantage of hardware acceleration provided by the GPU. If the GPU does not support the features needed for Direct2D, then Direct2D falls back to software rendering. Overall, **Direct2D outperforms GDI and GDI+ in most situations**.

**Direct2D supports fully hardware-accelerated alpha-blending (transparency).**

GDI has limited support for alpha-blending. Most GDI functions do not support alpha blending, although GDI does support alpha blending during a bitblt operation. GDI+ supports transparency, but the alpha blending is performed by the CPU, so it does not benefit from hardware acceleration.

Hardware-accelerated alpha-blending also enables anti-aliasing. GDI does not support anti-aliasing when it draws geometry (lines and curves). GDI can draw anti-aliased text using ClearType; but otherwise, GDI text is aliased as well. Aliasing is particularly noticeable for text, because the jagged lines disrupt the font design, making the text less readable. Although GDI+ supports anti-aliasing, it is applied by the CPU, so the performance is not as good as Direct2D.

### The Desktop Window Manager (DWM)

Before Windows Vista, a Windows program would draw **directly** to the screen. In other words, the program would write directly to the memory buffer shown by the video card. This approach can cause visual artifacts if a window does not repaint itself correctly. Windows Vista fundamentally changed how windows are drawn, by introducing the `Desktop Window Manager (DWM)`. When the DWM is enabled, a window no longer draws directly to the display buffer. Instead, each window draws to an offscreen memory buffer, also called an offscreen surface. The DWM then composites these surfaces to the screen.

![W/O DWM](/images/dwm_w_o.png)


#### The Windows Display Driver Model  (WDDM)

`The Windows Display Driver Model (WDDM)` is the graphics display driver architecture introduced in Windows Vista (WDDM v1.0). WDDM is required starting with Windows 8 (WDDM v1.2). The WDDM design guide discusses WDDM requirements, specifications, and behavior for WDDM drivers.

![DX10](/images/dx10arch.png)

A graphics hardware vendor must supply the user-mode display driver and the display miniport driver. The user-mode display driver is a dynamic-link library (DLL) that is loaded by the Microsoft Direct3D runtime. **The display miniport driver communicates with the Microsoft DirectX graphics kernel subsystem.**

### `DirectX`

`Microsoft DirectX Graphics Infrastructure (DXGI)` recognizes that some parts of graphics evolve more slowly than others. The primary goal of DXGI is to manage low-level tasks that can be independent of the DirectX graphics runtime. DXGI provides a common framework for future graphics components; the first component that takes advantage of DXGI is Microsoft Direct3D 10.

In previous versions of Direct3D, low-level tasks like enumeration of hardware devices, presenting rendered frames to an output, controlling gamma, and managing a full-screen transition were included in the Direct3D runtime. These tasks are now implemented in DXGI.

DXGI's purpose is to communicate with the kernel mode driver and the system hardware, as shown in the following diagram.

![DXGI](/images/dxgi-dll.png)

An application can access DXGI directly, or call the Direct3D APIs in D3D11_1.h, D3D11.h, D3D10_1.h, or D3D10.h, which handles the communications with DXGI for you. You may want to work with DXGI directly if your application needs to enumerate devices or control how data is presented to an output.


#### Desktop Duplication API

Windows 8 disables standard Windows 2000 Display Driver Model (XDDM) mirror drivers and offers the `desktop duplication API` instead. The desktop duplication API provides remote access to a desktop image for collaboration scenarios. **Apps can use the desktop duplication API to access frame-by-frame updates to the desktop**. Because apps receive updates to the desktop image in a DXGI surface, the apps can use the full power of the GPU to process the image updates.

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
- [录屏采集实现教程——Windows桌面端](https://juejin.cn/post/6975094080904790052)
- [Desktop Duplication API](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/desktop-dup-api)
- [WDDM](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/windows-vista-display-driver-model-design-guide)
- [New Ways to do Screen Capture](https://blogs.windows.com/windowsdeveloper/2019/09/16/new-ways-to-do-screen-capture/)
- [Win32CaptureSample](https://github.com/robmikh/Win32CaptureSample)
- [Windows.Graphics.Capture](https://learn.microsoft.com/en-us/uwp/api/windows.graphics.capture?view=winrt-22621)