#include "videoplayer.h"

VideoPlayer::VideoPlayer(QWidget* parent)
        : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);

    frame_ = av_frame_alloc();
    decoder_ = new MediaDecoder(this);
    decoder_->set_video_callback([=](AVFrame * frame) {
        mtx_.lock();
        if(frame_) {
            av_frame_unref(frame_);
            av_frame_ref(frame_, frame);
        }
        mtx_.unlock();

        QWidget::update();
    });

    resize(640, 480);
}

bool VideoPlayer::play(const std::string& name, const std::string& fmt, const std::string& filter_descr)
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