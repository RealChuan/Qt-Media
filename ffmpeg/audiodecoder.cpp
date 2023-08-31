#include "audiodecoder.h"
#include "avcontextinfo.h"
#include "decoderaudioframe.h"
#include "ffmpegutils.hpp"

#include <event/seekevent.hpp>

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

    void processEvent()
    {
        while (q_ptr->m_runing.load() && q_ptr->m_eventQueue.size() > 0) {
            auto eventPtr = q_ptr->m_eventQueue.take();
            switch (eventPtr->type()) {
            case Event::EventType::Pause: decoderAudioFrame->addEvent(eventPtr); break;
            case Event::EventType::Seek: {
                decoderAudioFrame->clear();
                decoderAudioFrame->addEvent(eventPtr);
                auto seekEvent = static_cast<SeekEvent *>(eventPtr.data());
                seekEvent->countDown();
            } break;
            default: break;
            }
        }
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

AudioDecoder::~AudioDecoder()
{
    stopDecoder();
}

void AudioDecoder::setVolume(qreal volume)
{
    d_ptr->decoderAudioFrame->setVolume(volume);
}

void AudioDecoder::setMasterClock()
{
    d_ptr->decoderAudioFrame->setMasterClock();
}

void AudioDecoder::runDecoder()
{
    d_ptr->decoderAudioFrame->startDecoder(m_formatContext, m_contextInfo);

    while (m_runing) {
        d_ptr->processEvent();

        auto packetPtr(m_queue.take());
        if (packetPtr.isNull()) {
            continue;
        }
        auto framePtrs = m_contextInfo->decodeFrame(packetPtr);
        for (const auto &framePtr : framePtrs) {
            calculatePts(framePtr.data(), m_contextInfo, m_formatContext);
            d_ptr->decoderAudioFrame->append(framePtr);
        }
    }
    while (m_runing && d_ptr->decoderAudioFrame->size() != 0) {
        msleep(s_waitQueueEmptyMilliseconds);
    }

    d_ptr->decoderAudioFrame->stopDecoder();
}

} // namespace Ffmpeg
