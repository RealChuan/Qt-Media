#include "audiodecoder.h"
#include "avcontextinfo.h"
#include "decoderaudioframe.h"

#include <QDebug>

namespace Ffmpeg {

class AudioDecoder::AudioDecoderPrivate
{
public:
    explicit AudioDecoderPrivate(AudioDecoder *q)
        : q_ptr(q)
    {
        decoderAudioFrame = new DecoderAudioFrame(q_ptr);
    }

    AudioDecoder *q_ptr;

    DecoderAudioFrame *decoderAudioFrame;
};

AudioDecoder::AudioDecoder(QObject *parent)
    : Decoder<PacketPtr>(parent)
    , d_ptr(new AudioDecoderPrivate(this))
{
    connect(d_ptr->decoderAudioFrame,
            &DecoderAudioFrame::positionChanged,
            this,
            &AudioDecoder::positionChanged);
}

AudioDecoder::~AudioDecoder() = default;

void AudioDecoder::seek(qint64 seekTime)
{
    Decoder<PacketPtr>::seek(seekTime);
    d_ptr->decoderAudioFrame->seek(seekTime);
}

void AudioDecoder::pause(bool state)
{
    d_ptr->decoderAudioFrame->pause(state);
}

void AudioDecoder::setVolume(qreal volume)
{
    d_ptr->decoderAudioFrame->setVolume(volume);
}

void AudioDecoder::setIsLocalFile(bool isLocalFile)
{
    d_ptr->decoderAudioFrame->setIsLocalFile(isLocalFile);
}

void AudioDecoder::runDecoder()
{
    d_ptr->decoderAudioFrame->startDecoder(m_formatContext, m_contextInfo);

    while (m_runing) {
        auto packetPtr(m_queue.take());
        if (packetPtr.isNull()) {
            continue;
        }
        auto frames(m_contextInfo->decodeFrame(packetPtr.data()));
        for (auto frame : frames) {
            Ffmpeg::calculateTime(frame, m_contextInfo, m_formatContext);
            d_ptr->decoderAudioFrame->append(FramePtr(frame));
        }
    }
    while (m_runing && d_ptr->decoderAudioFrame->size() != 0) {
        msleep(Sleep_Queue_Full_Milliseconds);
    }

    d_ptr->decoderAudioFrame->stopDecoder();
}

} // namespace Ffmpeg
