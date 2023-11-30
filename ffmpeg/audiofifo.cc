#include "audiofifo.hpp"
#include "averrormanager.hpp"
#include "codeccontext.h"

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/audio_fifo.h>
}

namespace Ffmpeg {

class AudioFifo::AudioFifoPrivtate
{
public:
    explicit AudioFifoPrivtate(AudioFifo *q)
        : q_ptr(q)
    {}

    ~AudioFifoPrivtate()
    {
        if (audioFifo != nullptr) {
            av_audio_fifo_free(audioFifo);
        }
    }

    AudioFifo *q_ptr;

    AVAudioFifo *audioFifo = nullptr;
};

AudioFifo::AudioFifo(CodecContext *ctx, QObject *parent)
    : QObject{parent}
    , d_ptr(new AudioFifoPrivtate(this))
{
    auto *avCodecCtx = ctx->avCodecCtx();
    d_ptr->audioFifo = av_audio_fifo_alloc(avCodecCtx->sample_fmt, ctx->chLayout().nb_channels, 1);
    Q_ASSERT(nullptr != d_ptr->audioFifo);
}

AudioFifo::~AudioFifo() = default;

auto AudioFifo::realloc(int nb_samples) -> bool
{
    auto ret = av_audio_fifo_realloc(d_ptr->audioFifo, nb_samples);
    ERROR_RETURN(ret)
}

auto AudioFifo::write(void **data, int nb_samples) -> bool
{
    auto ret = av_audio_fifo_write(d_ptr->audioFifo, data, nb_samples);
    if (ret < nb_samples) {
        qWarning() << "Could not write data to FIFO";
        return false;
    }
    return true;
}

auto AudioFifo::read(void **data, int nb_samples) -> bool
{
    auto ret = av_audio_fifo_read(d_ptr->audioFifo, data, nb_samples);
    if (ret < nb_samples) {
        qWarning() << "Could not read data from FIFO";
        return false;
    }
    return true;
}

auto AudioFifo::size() const -> int
{
    return av_audio_fifo_size(d_ptr->audioFifo);
}

} // namespace Ffmpeg
