#include <Unknwn.h>
#include <inspectable.h>

// WinRT
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.UI.Popups.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>

#include <windows.ui.composition.interop.h>
#include <DispatcherQueue.h>

// STL
#include <atomic>
#include <memory>

// D3D
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d2d1_3.h>
#include <wincodec.h>

#include <thread>
#include "logging.h"

#include "dwmapi.h"
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.capture.h>

#include "SimpleCapture.h"
#include "d3dHelpers.h"
#include "direct3d11.interop.h"

#include <chrono>

using namespace std::chrono_literals;

struct window_t {
    HWND id;

    std::wstring title;
    std::wstring class_name;
    RECT rect;

    bool visible;
};


window_t get_window_info(HWND hwnd)
{
    window_t info{};

    info.id = hwnd;

    // title
    auto len = GetWindowTextLength(hwnd);
    if (len > 0) {
        info.title.resize(len + 1, 0);

        GetWindowText(hwnd, info.title.data(), len + 1);
    }

    // class name
    info.class_name.resize(256, 0);
    GetClassName(hwnd, info.class_name.data(), 256);

    // rect
    DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &info.rect, sizeof(RECT));

    // visible
    BOOL is_cloaked = false;
    info.visible = IsWindowVisible(hwnd) &&
        SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &is_cloaked, sizeof(is_cloaked))) &&
        !is_cloaked &&
        !(GetWindowLongW(hwnd, GWL_STYLE) & WS_DISABLED);

    return info;
}

std::vector<window_t> enum_windows()
{
    std::vector<window_t> windows;

    for (auto hwnd = GetTopWindow(nullptr); hwnd != nullptr; hwnd = GetNextWindow(hwnd, GW_HWNDNEXT)) {
        auto window = get_window_info(hwnd);

        if (window.visible) {
            windows.push_back(window);
        }
    }

    return windows;
}

winrt::Windows::Graphics::Capture::GraphicsCaptureItem create_capture_item_for(HWND hwnd)
{
    auto activation_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
    auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
    interop_factory->CreateForWindow(hwnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), reinterpret_cast<void**>(winrt::put_abi(item)));
    return item;
}

int main(int argc, char *argv[])
{
    if (!winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported()) {
        LOG(ERROR) << "unsupported";
        return -1;
    }

    winrt::Windows::Graphics::Capture::GraphicsCaptureSession capture_session{ nullptr };
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice d3d_device{ nullptr };

    auto windows = enum_windows();

    auto d3dDevice = CreateD3DDevice();
    auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
    d3d_device = CreateDirect3DDevice(dxgiDevice.get());

    std::unique_ptr<SimpleCapture> capturer{ nullptr };

    
    for (auto window : windows) {
        if (window.title.find(L"Google Chrome") != std::wstring::npos) {
            capturer = std::make_unique<SimpleCapture>(d3d_device, create_capture_item_for(window.id));
            break;
        }
    }

    if (capturer != nullptr) {
        capturer->StartCapture();
    }

    for (;;) {
        std::this_thread::sleep_for(1000ms);
    }


    return 0;
}