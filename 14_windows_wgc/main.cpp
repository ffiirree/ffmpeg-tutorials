#include "defer.h"
#include "dwmapi.h"
#include "fmt/format.h"
#include "logging.h"
#include "wgc-capturer.h"

#include <chrono>
#include <memory>
#include <thread>

extern "C" {
#include <libavutil/pixdesc.h>
}

using namespace std::chrono_literals;

struct window_t
{
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
    info.visible    = IsWindowVisible(hwnd) &&
                   SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &is_cloaked, sizeof(is_cloaked))) &&
                   !is_cloaked && !(GetWindowLongW(hwnd, GWL_STYLE) & WS_DISABLED);

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

AVBufferRef *device_ref = nullptr;
AVBufferRef *frames_ref = nullptr;

static int init_hwframes_ctx(int w, int h)
{
    CHECK_NOTNULL(frames_ref = av_hwframe_ctx_alloc(device_ref));

    auto frames_ctx = reinterpret_cast<AVHWFramesContext *>(frames_ref->data);

    frames_ctx->format    = AV_PIX_FMT_D3D11;
    frames_ctx->height    = h;
    frames_ctx->width     = w;
    frames_ctx->sw_format = AV_PIX_FMT_BGRA;

    if (av_hwframe_ctx_init(frames_ref) < 0) {
        LOG(ERROR) << "[      WGC] av_hwframe_ctx_init";
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    Logger::init(argv[0]);

    winrt::init_apartment();
    defer(winrt::uninit_apartment());

    if (!winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported()) {
        LOG(ERROR) << "Windows::Graphics::Capture is not supported.";
        return -1;
    }

    std::string output_filename = "wgc-output.mp4";

    // hardware device, wrapper to d3d11device
    if (av_hwdevice_ctx_create(&device_ref, AV_HWDEVICE_TYPE_D3D11VA, nullptr, nullptr, 0) < 0) {
        return -1;
    };

    auto capturer = std::make_unique<WgcCapturer>();

    // set hardware device for WGC
    capturer->hw_device_ref_ = av_buffer_ref(device_ref);

    for (const auto& window : enum_windows()) {
        if (window.title.find(L"Visual Studio Code") != std::wstring::npos) {
            capturer->open(window.id);
            break;
        }
    }

    // frame pool
    init_hwframes_ctx(capturer->last_size_.Width, capturer->last_size_.Height);
    capturer->hw_frames_ref_ = av_buffer_ref(frames_ref);

    auto start_time = av_gettime_relative();
    if (capturer != nullptr) {
        capturer->StartCapture();
    }

    //
    // output
    //
    AVFormatContext *encoder_fmt_ctx = nullptr;
    CHECK(avformat_alloc_output_context2(&encoder_fmt_ctx, nullptr, nullptr, output_filename.c_str()) >= 0);
    defer(avformat_free_context(encoder_fmt_ctx));

    // encoder
    auto encoder = avcodec_find_encoder_by_name("h264_nvenc");
    CHECK_NOTNULL(encoder);

    AVCodecContext *encoder_ctx = avcodec_alloc_context3(encoder);
    CHECK_NOTNULL(encoder_ctx);
    defer(avcodec_free_context(&encoder_ctx));
    encoder_ctx->codec_type = AVMEDIA_TYPE_VIDEO;

    encoder_ctx->hw_frames_ctx = av_buffer_ref(frames_ref);
    CHECK(encoder_ctx->hw_frames_ctx);

    encoder_ctx->hw_device_ctx = av_buffer_ref(device_ref);
    CHECK(encoder_ctx->hw_device_ctx);

    // encoder codec params
    encoder_ctx->height              = capturer->last_size_.Height;
    encoder_ctx->width               = capturer->last_size_.Width;
    encoder_ctx->pix_fmt             = AV_PIX_FMT_D3D11;
    encoder_ctx->sample_aspect_ratio = { 1, 1 };
    // time base
    encoder_ctx->time_base           = { 1, 30 };
    encoder_ctx->framerate           = { 30, 1 };

    CHECK(avcodec_open2(encoder_ctx, encoder, nullptr) >= 0);
    CHECK(avformat_new_stream(encoder_fmt_ctx, encoder));
    CHECK(avcodec_parameters_from_context(encoder_fmt_ctx->streams[0]->codecpar, encoder_ctx) >= 0);

    if (!(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        CHECK(avio_open(&encoder_fmt_ctx->pb, output_filename.c_str(), AVIO_FLAG_WRITE) >= 0);
    }

    encoder_fmt_ctx->streams[0]->time_base      = { 1, AV_TIME_BASE };
    encoder_fmt_ctx->streams[0]->avg_frame_rate = { 30, 1 };

    CHECK(avformat_write_header(encoder_fmt_ctx, nullptr) >= 0);

    av_dump_format(encoder_fmt_ctx, 0, output_filename.c_str(), 1);
    LOG(INFO) << fmt::format(
        "[OUTPUT] {}x{}, encoder: {}, format: {}, framerate: {}/{}, tbc: {}/{}, tbn: {}/{}",
        encoder_ctx->width, encoder_ctx->height, encoder->name, av_get_pix_fmt_name(encoder_ctx->pix_fmt),
        encoder_ctx->framerate.num, encoder_ctx->framerate.den, encoder_ctx->time_base.num,
        encoder_ctx->time_base.den, encoder_fmt_ctx->streams[0]->time_base.num,
        encoder_fmt_ctx->streams[0]->time_base.den);

    AVFrame *frame   = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();
    defer(av_frame_free(&frame));
    defer(av_packet_free(&packet));

    auto last_pts    = AV_NOPTS_VALUE;
    uint32_t counter = 0;
    for (size_t i = 180; i;) {
        if (capturer->next(frame) == AVERROR(EAGAIN)) {
            std::this_thread::sleep_for(25ms);
            continue;
        }

        --i;

        // clear
        frame->pict_type = AV_PICTURE_TYPE_NONE;
        frame->pts       = counter++;

        // encoding
        auto ret = avcodec_send_frame(encoder_ctx, frame);
        while (ret >= 0) {
            av_packet_unref(packet);
            ret = avcodec_receive_packet(encoder_ctx, packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            else if (ret < 0) {
                LOG(ERROR) << "decoding error";
                return ret;
            }

            packet->stream_index = 0;
            av_packet_rescale_ts(packet, encoder_ctx->time_base, encoder_fmt_ctx->streams[0]->time_base);

            LOG(INFO) << fmt::format(
                " -- [ENCODING] frame = {:>5d}, pts = {:>10d}, dts = {:>10d}, size = {:>6d}",
                encoder_ctx->frame_number, packet->pts, packet->dts, packet->size);

            if (av_interleaved_write_frame(encoder_fmt_ctx, packet) != 0) {
                LOG(ERROR) << "encoder: av_interleaved_write_frame()";
                return -1;
            }
        }
    }

    av_write_trailer(encoder_fmt_ctx);
    if (encoder_fmt_ctx && !(encoder_fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&encoder_fmt_ctx->pb);

    return 0;
}