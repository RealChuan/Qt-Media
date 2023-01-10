#include "audiofifo.hpp"
#include "averrormanager.hpp"
#include "codeccontext.h"

#include <QDebug>

extern "C" {
#include "libavutil/audio_fifo.h"
}

namespace Ffmpeg {

class AudioFifo::AudioFifoPrivtate
{
public:
    AudioFifoPrivtate(QObject *parent)
        : owner(parent)
    {}

    ~AudioFifoPrivtate()
    {
        if (audioFifo) {
            av_audio_fifo_free(audioFifo);
        }
    }

    void setError(int errorCode) { AVErrorManager::instance()->setErrorCode(errorCode); }

    QObject *owner;

    AVAudioFifo *audioFifo = nullptr;
};

AudioFifo::AudioFifo(CodecContext *ctx, QObject *parent)
    : QObject{parent}
    , d_ptr(new AudioFifoPrivtate(this))
{
    d_ptr->audioFifo = av_audio_fifo_alloc(ctx->sampleFmt(), ctx->channels(), 1);
    Q_ASSERT(nullptr != d_ptr->audioFifo);
}

AudioFifo::~AudioFifo() {}

bool AudioFifo::realloc(int nb_samples)
{
    auto ret = av_audio_fifo_realloc(d_ptr->audioFifo, nb_samples);
    if (ret < 0) {
        d_ptr->setError(ret);
        return false;
    }
    return true;
}

bool AudioFifo::write(void **data, int nb_samples)
{
    auto ret = av_audio_fifo_write(d_ptr->audioFifo, data, nb_samples);
    if (ret < nb_samples) {
        qWarning() << "Could not write data to FIFO";
        return false;
    }
    return true;
}

bool AudioFifo::read(void **data, int nb_samples)
{
    auto ret = av_audio_fifo_read(d_ptr->audioFifo, data, nb_samples);
    if (ret < nb_samples) {
        qWarning() << "Could not read data from FIFO";
        return false;
    }
    return true;
}

int AudioFifo::size() const
{
    return av_audio_fifo_size(d_ptr->audioFifo);
}

} // namespace Ffmpeg
