#ifndef PLAYER_VIDEO_PLAYER_H
#define PLAYER_VIDEO_PLAYER_H

#include <QWidget>
#include <QPainter>
#include <QImage>
#include "mediadecoder.h"
#include "logging.h"
#include "fmt/format.h"
#include "audioplayer.h"
#include "ringbuffer.h"

class VideoPlayer : public QWidget {
    Q_OBJECT

public:
	explicit VideoPlayer(QWidget* parent = nullptr)
        : QWidget(nullptr)
	{
        setAttribute(Qt::WA_TranslucentBackground);

        frame_ = av_frame_alloc();

        decoder_ = new MediaDecoder(this);

        audio_player_ = new AudioPlayer(this);
        audio_player_->open(48000, 2, 16);

        decoder_->set_video_callback([=](AVFrame * frame) {
            mtx_.lock();
            if(frame_) {
                av_frame_unref(frame_);
                av_frame_ref(frame_, frame);
            }
            mtx_.unlock();

            QWidget::update();
        });

        decoder_->set_period_size(audio_player_->period_size());
        decoder_->set_audio_callback([=](RingBuffer& buffer) -> std::pair<int64_t, bool> {
            bool ok = false;

            if (buffer.continuous_size() >= audio_player_->period_size() 
                && audio_player_->buffer_free_size() >= audio_player_->period_size()) {
                size_t read_size = audio_player_->period_size();
                audio_player_->write(buffer.read_ptr(read_size), audio_player_->period_size());

                ok = true;
            }

            return std::pair{ audio_player_->buffered_size(), ok };
        });

        resize(640, 480);
	}

    ~VideoPlayer() override { av_frame_free(&frame_); };

    bool play(const std::string& name, const std::string& fmt = "", const std::string& filter_descr = "")
    {
        if (!decoder_->open(name, fmt, filter_descr, AV_PIX_FMT_RGB24, {})) {
            return false;
        }

        resize((decoder_->width() > 1440 || decoder_->height() > 810) ?
            QSize(decoder_->width(), decoder_->height()).scaled(1440, 810, Qt::KeepAspectRatio) :
            QSize( decoder_->width(), decoder_->height())
        );

        decoder_->start();

        QWidget::setWindowTitle(QString::fromStdString(name));

        return true;
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        std::lock_guard lock(mtx_);

        if(frame_ && frame_->format == AV_PIX_FMT_RGB24) {
            QPainter painter(this);
            painter.drawImage(rect(), QImage(static_cast<const uchar*>(frame_->data[0]), frame_->width, frame_->height, QImage::Format_RGB888));
        }
    }

    MediaDecoder* decoder_{ nullptr };
    AudioPlayer * audio_player_{nullptr};

    AVFrame *frame_{ nullptr };
    std::mutex mtx_;
};

#endif // !PLAYER_VIDEO_PLAYER_H