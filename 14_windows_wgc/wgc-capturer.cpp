#include <Unknwn.h>
#include <inspectable.h>
#include "wgc-capturer.h"
#include "direct3d11.interop.h"
#include "d3dHelpers.h"
#include "logging.h"

using namespace winrt;
using namespace Windows;
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::Graphics;
using namespace Windows::Graphics::Capture;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI;
using namespace Windows::UI::Composition;

static auto create_d3d_device()
{
    auto d3d_device = CreateD3DDevice();
    auto dxgi_device = d3d_device.as<IDXGIDevice>();

    winrt::com_ptr<::IInspectable> inspectable;
    winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi_device.get(), inspectable.put()));

    return inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
}

int WgcCapturer::open(HWND hwnd, HMONITOR monitor)
{
    if (hwnd) {
        item_ = GraphicsCaptureItem::TryCreateFromWindowId(WindowId{reinterpret_cast<uint64_t>(hwnd)});
    }
    else if (monitor) {
        item_ = GraphicsCaptureItem::TryCreateFromDisplayId(DisplayId{reinterpret_cast<uint64_t>(monitor)});
    }

    if (!item_) {
        LOG(ERROR) << "[     WGC] can not create GraphicsCaptureItem.";
        return -1;
    }

    device_ = create_d3d_device();

	// Set up 
    auto dxgi_device = GetDXGIInterfaceFromObject<ID3D11Device>(device_);
    dxgi_device->GetImmediateContext(d3d11_context_.put());

    frame_pool_ = Direct3D11CaptureFramePool::CreateFreeThreaded(
            device_,
            DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            item_.Size()
    );
    last_size_ = item_.Size();
	frame_arrived_ = frame_pool_.FrameArrived(auto_revoke, { this, &WgcCapturer::OnFrameArrived });
    session_ = frame_pool_.CreateCaptureSession(item_);

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
    if (running_.compare_exchange_strong(expected, false))
    {
		frame_arrived_.revoke();
		frame_pool_.Close();
        session_.Close();

        frame_pool_ = nullptr;
        session_ = nullptr;
        item_ = nullptr;

        av_buffer_unref(&hw_device_ref_);
        av_buffer_unref(&hw_frames_ref_);

        av_frame_free(&frame_);
    }
}

void WgcCapturer::OnFrameArrived(Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const&)
{
    auto frame = sender.TryGetNextFrame();
    auto frame_size = frame.ContentSize();
    auto frame_surface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());

    if (frame_size.Width != last_size_.Width || frame_size.Height != last_size_.Height)
    {
        frame_pool_.Recreate(
                device_,
                DirectXPixelFormat::B8G8R8A8UIntNormalized,
                2,
                frame_size
        );

        last_size_ = frame_size;
    }

    // copy
    av_frame_unref(frame_);
    if (av_hwframe_get_buffer(reinterpret_cast<AVBufferRef*>(hw_frames_ref_), frame_, 0) < 0) {
        LOG(ERROR) << "av_hwframe_get_buffer";
        return;
    }

    frame_->sample_aspect_ratio = { 1, 1 };
    frame_->pts = av_gettime_relative();

    frame_->color_range = AVCOL_RANGE_JPEG;
    frame_->color_primaries = AVCOL_PRI_BT709;
    frame_->color_trc = AVCOL_TRC_IEC61966_2_1;
    frame_->colorspace = AVCOL_SPC_RGB;

    d3d11_context_->CopyResource(reinterpret_cast<ID3D11Texture2D*>(frame_->data[0]), frame_surface.get());

    LOG(INFO) << "frame arrived: " << frame_number_++;

    buffer_.push([this](AVFrame* frame) {
        av_frame_unref(frame);
        av_frame_move_ref(frame, frame_);
    });
}

