#include "audiodecoder.h"
#include "avcontextinfo.h"
#include "decoderaudioframe.h"

#include <QDebug>

namespace Ffmpeg {

class AudioDecoderPrivate
{
public:
    AudioDecoderPrivate(QObject *parent)
        : owner(parent)
    {
        decoderAudioFrame = new DecoderAudioFrame(owner);
    }
    QObject *owner;

    DecoderAudioFrame *decoderAudioFrame;
};

AudioDecoder::AudioDecoder(QObject *parent)
    : Decoder<Packet *>(parent)
    , d_ptr(new AudioDecoderPrivate(this))
{
    connect(d_ptr->decoderAudioFrame,
            &DecoderAudioFrame::positionChanged,
            this,
            &AudioDecoder::positionChanged);
}

AudioDecoder::~AudioDecoder() {}

void AudioDecoder::pause(bool state)
{
    d_ptr->decoderAudioFrame->pause(state);
}

void AudioDecoder::setVolume(qreal volume)
{
    d_ptr->decoderAudioFrame->setVolume(volume);
}

void AudioDecoder::setSpeed(double speed)
{
    Decoder<Packet *>::setSpeed(speed);
    d_ptr->decoderAudioFrame->setSpeed(speed);
}

double AudioDecoder::audioClock()
{
    return d_ptr->decoderAudioFrame->audioClock();
}

void AudioDecoder::setIsLocalFile(bool isLocalFile)
{
    d_ptr->decoderAudioFrame->setIsLocalFile(isLocalFile);
}

void AudioDecoder::runDecoder()
{
    d_ptr->decoderAudioFrame->startDecoder(m_formatContext, m_contextInfo);

    while (m_runing) {
        if (m_seek) {
            d_ptr->decoderAudioFrame->seek(m_seekTime);
            seekFinish();
        }

        if (m_queue.isEmpty()) {
            msleep(1);
            continue;
        }

        QScopedPointer<Packet> packetPtr(m_queue.dequeue());

        if (!m_contextInfo->sendPacket(packetPtr.data())) {
            continue;
        }

        std::unique_ptr<PlayFrame> framePtr(new PlayFrame);
        while (m_contextInfo->receiveFrame(framePtr.get())) { // 一个packet 一个或多个音频帧
            d_ptr->decoderAudioFrame->append(framePtr.release());
            framePtr.reset(new PlayFrame);
        }

        while (m_runing && d_ptr->decoderAudioFrame->size() > 10 && !m_seek) {
            msleep(1);
        }
    }
    while (m_runing && d_ptr->decoderAudioFrame->size() != 0) {
        msleep(40);
    }

    d_ptr->decoderAudioFrame->stopDecoder();
}

} // namespace Ffmpeg
