#include "wgc-capturer.h"

#include "defer.h"
#include "fmt/format.h"
#include "logging.h"

using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;

int WgcCapturer::open(HWND hwnd, HMONITOR monitor)
{
    if (hw_device_ref_ == nullptr) {
        LOG(ERROR) << "[     WGC] please set hardware device first.";
        return -1;
    }

    // parse capture item
    if (hwnd) {
        item_ = wgc::create_capture_item_for_window(hwnd);
    }
    else if (monitor) {
        item_ = wgc::create_capture_item_for_monitor(monitor);
    }

    if (!item_) {
        LOG(ERROR) << "[     WGC] can not create GraphicsCaptureItem.";
        return -1;
    }

    // D3D11Device & ImmediateContext
    // winrt::com_ptr<::ID3D11Device> d3d11_device = wgc::create_d3d11device();
    AVHWDeviceContext *device_ctx = reinterpret_cast<AVHWDeviceContext *>(hw_device_ref_->data);
    ID3D11Device *d3d11_device    = reinterpret_cast<AVD3D11VADeviceContext *>(device_ctx->hwctx)->device;
    d3d11_device->GetImmediateContext(d3d11_context_.put());

    // If you're using C++/WinRT, then to move back and forth between IDirect3DDevice and IDXGIDevice, use
    // the IDirect3DDxgiInterfaceAccess::GetInterface and CreateDirect3D11DeviceFromDXGIDevice functions.
    //
    // This represents an IDXGIDevice, and can be used to interop between Windows Runtime components that
    // need to exchange IDXGIDevice references.
    winrt::com_ptr<::IInspectable> inspectable{};
    winrt::com_ptr<IDXGIDevice> dxgi_device{};
    d3d11_device->QueryInterface(winrt::guid_of<IDXGIDevice>(), dxgi_device.put_void());
    if (FAILED(::CreateDirect3D11DeviceFromDXGIDevice(dxgi_device.get(), inspectable.put()))) {
        LOG(ERROR) << "[     WGC] CreateDirect3D11DeviceFromDXGIDevice failed.";
        return -1;
    }
    device_ = inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();

    frame_pool_ = Direct3D11CaptureFramePool::CreateFreeThreaded(
        device_, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, item_.Size());

    last_size_ = item_.Size();

    frame_arrived_ = frame_pool_.FrameArrived(winrt::auto_revoke, { this, &WgcCapturer::OnFrameArrived });

    session_ = frame_pool_.CreateCaptureSession(item_);
    // draw mouse
    session_.IsCursorCaptureEnabled(true);
    // show recording region
    session_.IsBorderRequired(true);

    frame_ = av_frame_alloc();
    return 0;
}

// Start sending capture frames
void WgcCapturer::StartCapture()
{
    if (!running_) {
        running_ = true;
        session_.StartCapture();
    }
}

// Process captured frames
void WgcCapturer::Close()
{
    auto expected = true;
    if (running_.compare_exchange_strong(expected, false)) {
        frame_arrived_.revoke();
        frame_pool_.Close();
        session_.Close();

        frame_pool_ = nullptr;
        session_    = nullptr;
        item_       = nullptr;

        av_buffer_unref(&hw_device_ref_);
        av_buffer_unref(&hw_frames_ref_);

        av_frame_free(&frame_);
    }
}

void WgcCapturer::OnFrameArrived(Direct3D11CaptureFramePool const& sender,
                                 winrt::Windows::Foundation::IInspectable const&)
{
    auto frame         = sender.TryGetNextFrame();
    auto frame_size    = frame.ContentSize();
    auto frame_surface = wgc::get_interface_from<::ID3D11Texture2D>(frame.Surface());

    if (frame_size.Width != last_size_.Width || frame_size.Height != last_size_.Height) {
        frame_pool_.Recreate(device_, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, frame_size);

        last_size_ = frame_size;
    }

    // copy
    av_frame_unref(frame_);
    if (av_hwframe_get_buffer(reinterpret_cast<AVBufferRef *>(hw_frames_ref_), frame_, 0) < 0) {
        LOG(ERROR) << "av_hwframe_get_buffer";
        return;
    }

    frame_->sample_aspect_ratio = { 1, 1 };
    frame_->pts                 = av_gettime_relative();

    d3d11_context_->CopyResource(reinterpret_cast<ID3D11Texture2D *>(frame_->data[0]), frame_surface.get());

    LOG(INFO) << "frame arrived: " << frame_number_++;

    buffer_.push([=, this](AVFrame *frame) {
        av_frame_unref(frame);
        av_frame_move_ref(frame, frame_);
    });
}
