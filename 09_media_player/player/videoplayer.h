#ifndef PLAYER_VIDEO_PLAYER_H
#define PLAYER_VIDEO_PLAYER_H

#include <QWidget>
#include <QPainter>
#include <QImage>
#include "mediadecoder.h"
#include "logging.h"
#include "fmt/format.h"
#include "audioplayer.h"

class VideoPlayer : public QWidget {
    Q_OBJECT

public:
    explicit VideoPlayer(QWidget* parent = nullptr);
    ~VideoPlayer() override { av_frame_free(&frame_); };

    bool play(const std::string& name, const std::string& fmt = "", const std::string& filter_descr = "");

protected:
    void paintEvent(QPaintEvent* event) override
    {
        std::lock_guard lock(mtx_);

        if(frame_ && frame_->format == AV_PIX_FMT_BGRA) {
            QPainter painter(this);
            painter.drawImage(rect(), QImage(static_cast<const uchar*>(frame_->data[0]), frame_->width, frame_->height, QImage::Format_ARGB32));
        }
    }

    MediaDecoder* decoder_{ nullptr };
    AudioPlayer * audio_player_{nullptr};

    AVFrame *frame_{ nullptr };
    std::mutex mtx_;
};

#endif // !PLAYER_VIDEO_PLAYER_H