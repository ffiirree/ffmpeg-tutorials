#ifndef WGC_CAPTURER_H
#define WGC_CAPTURER_H

#include "win-wgc.h"

extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavutil/hwcontext_d3d11va.h>
}

#include "ringvector.h"

class WgcCapturer
{
public:
    ~WgcCapturer() { Close(); }

    int open(HWND hwnd, HMONITOR monitor = nullptr);

    void StartCapture();

    void Close();

    winrt::Windows::Graphics::SizeInt32 last_size_{};

    AVBufferRef* hw_device_ref_{ nullptr };
    AVBufferRef* hw_frames_ref_{ nullptr };

    int next(AVFrame* frame)
    {
        if (buffer_.empty()) {
            return AVERROR(EAGAIN);
        }

        buffer_.pop([frame](AVFrame* popped) {
            av_frame_unref(frame);
            av_frame_move_ref(frame, popped);
        });
        return 0;
    }

private:
    void OnFrameArrived(
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
        winrt::Windows::Foundation::IInspectable const& args);

private:
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item_{ nullptr };
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool frame_pool_{ nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession session_{ nullptr };

    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice device_{ nullptr };
    winrt::com_ptr<ID3D11DeviceContext> d3d11_context_{ nullptr };

    std::atomic<bool> running_{};
	winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker frame_arrived_;


    AVFrame* frame_{ nullptr };
    uint32_t frame_number_{};

    RingVector<AVFrame*, 8> buffer_{
        []() { return av_frame_alloc(); },
        [](AVFrame** frame) { av_frame_free(frame); }
    };
};

#endif //!WGC_CAPTURER_H