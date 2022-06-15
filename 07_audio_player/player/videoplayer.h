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

    bool play(const std::string& name);

protected:
    MediaDecoder* decoder_{ nullptr };
    AudioPlayer * audio_player_{nullptr};
};

#endif // !PLAYER_VIDEO_PLAYER_H