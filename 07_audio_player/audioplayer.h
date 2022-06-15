#ifndef PLAYER_AUDIO_PLAYER_H
#define PLAYER_AUDIO_PLAYER_H

#include <QAudioOutput>
#include <QAudioFormat>
#include "logging.h"

class AudioPlayer {
public:
    ~AudioPlayer()
    {
        if(audio_output_) {
            audio_output_->stop();
            delete audio_output_;
            audio_output_ = nullptr;
        }
    }

    int open(int sample_rate, int channels, int sample_size)
    {
        QAudioFormat format;
        format.setSampleRate(sample_rate);
        format.setChannelCount(channels);
        format.setSampleSize(sample_size);
        format.setCodec("audio/pcm");
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::SignedInt);

        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
        if (!info.isFormatSupported(format)) {
            LOG(ERROR) << "not support the audio format\n";
            return -1;
        }

        audio_output_ = new QAudioOutput(format);
        audio_output_->setBufferSize(4096 * 10);
        audio_io_ = audio_output_->start();

        if (!audio_io_) {
            LOG(ERROR) << "audio_io_ is nullptr";
            return -1;
        }

        return 0;
    }

    int buffer_free_size()
    {
        return audio_output_ ? audio_output_->bytesFree() : 0;
    }

    int period_size()
    {
        return audio_output_ ? audio_output_->periodSize(): 0;
    }

    int64_t write(const char * ptr, int64_t size)
    {
        return audio_io_->write(ptr, size);
    }

private:
    QAudioOutput* audio_output_{nullptr};
    QIODevice * audio_io_{nullptr};
};

#endif // !PLAYER_AUDIO_PLAYER_H