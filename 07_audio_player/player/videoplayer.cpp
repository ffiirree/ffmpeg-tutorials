#include "videoplayer.h"
#include "ringbuffer.h"

VideoPlayer::VideoPlayer(QWidget* parent)
        : QWidget(parent)
{
    decoder_ = new MediaDecoder(this);

    audio_player_ = new AudioPlayer(this);
    audio_player_->open(48000, 2, 16);

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

    resize(640, 240);
}

bool VideoPlayer::play(const std::string& name)
{
    if (!decoder_->open(name)) {
        return false;
    }

    decoder_->start();

    QWidget::setWindowTitle("Playing");

    return true;
}