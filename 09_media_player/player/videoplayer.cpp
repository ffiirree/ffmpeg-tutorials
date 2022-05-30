#include "videoplayer.h"
#include "ringbuffer.h"
#include <QMessageBox>

VideoPlayer::VideoPlayer(QWidget* parent)
        : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);

    frame_ = av_frame_alloc();

    decoder_ = new MediaDecoder(this);

    audio_player_ = new AudioPlayer(this);

    decoder_->set_video_callback([=](AVFrame * frame) {
        mtx_.lock();
        if(frame_) {
            av_frame_unref(frame_);
            av_frame_ref(frame_, frame);
        }
        mtx_.unlock();

        QWidget::update();
    });

    decoder_->set_audio_callback([=](RingBuffer& buffer) -> std::pair<int64_t, bool> {
        bool ok = false;

        if (buffer.continuous_size() >= audio_player_->period_size() &&
            audio_player_->buffer_free_size() >= audio_player_->period_size()) {

            size_t read_size = audio_player_->period_size();
            audio_player_->write(buffer.read_ptr(read_size), audio_player_->period_size());

            ok = true;
        }

        return std::pair{ audio_player_->buffered_size(), ok };
    });

    resize(640, 480);
}

bool VideoPlayer::play(const std::string& name, const std::string& fmt, const std::string& filter_descr)
{
    if (!decoder_->open(name, fmt, filter_descr, AV_PIX_FMT_BGRA, {})) {
        QMessageBox::warning(this, "Error", QString::fromStdString({"Open " + name + " failed!"}));
        return false;
    }

    if (audio_player_->open(48000, 2, 16) != 0) {
        QMessageBox::warning(this, "Error", QString::fromStdString({"Not supported audio format!"}));
        return false;
    }

    decoder_->set_period_size(audio_player_->period_size());

    resize((decoder_->width() > 1440 || decoder_->height() > 810) ?
           QSize(decoder_->width(), decoder_->height()).scaled(1440, 810, Qt::KeepAspectRatio) :
           QSize( decoder_->width(), decoder_->height())
    );

    decoder_->start();

    QWidget::setWindowTitle(QString::fromStdString(name));

    return true;
}
