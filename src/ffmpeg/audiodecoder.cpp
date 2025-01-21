#include "audiodecoder.h"
#include "audiodisplay.hpp"
#include "avcontextinfo.h"
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
        audioDisplay = new AudioDisplay(q_ptr);
    }

    void processEvent() const
    {
        while (q_ptr->m_runing.load() && !q_ptr->m_eventQueue.isEmpty()) {
            auto eventPtr = q_ptr->m_eventQueue.take();
            switch (eventPtr->type()) {
            case Event::EventType::Pause: audioDisplay->addEvent(eventPtr); break;
            case Event::EventType::Seek: {
                auto *seekEvent = static_cast<SeekEvent *>(eventPtr.data());
                seekEvent->countDown();
                q_ptr->clear();
                audioDisplay->addEvent(eventPtr);
            } break;
            default: break;
            }
        }
    }

    AudioDecoder *q_ptr;

    AudioDisplay *audioDisplay;
};

AudioDecoder::AudioDecoder(QObject *parent)
    : Decoder<PacketPtr>(parent)
    , d_ptr(new AudioDecoderPrivate(this))
{
    connect(d_ptr->audioDisplay,
            &AudioDisplay::positionChanged,
            this,
            &AudioDecoder::positionChanged);
}

AudioDecoder::~AudioDecoder()
{
    stopDecoder();
}

void AudioDecoder::setVolume(qreal volume)
{
    d_ptr->audioDisplay->setVolume(volume);
}

void AudioDecoder::setMasterClock()
{
    d_ptr->audioDisplay->setMasterClock();
}

void AudioDecoder::runDecoder()
{
    d_ptr->audioDisplay->startDecoder(m_formatContext, m_contextInfo);

    while (m_runing.load()) {
        d_ptr->processEvent();

        auto packetPtr(m_queue.take());
        if (packetPtr.isNull()) {
            continue;
        }
        auto framePtrs = m_contextInfo->decodeFrame(packetPtr);
        for (const auto &framePtr : std::as_const(framePtrs)) {
            calculatePts(framePtr.data(), m_contextInfo, m_formatContext);
            d_ptr->audioDisplay->append(framePtr);
        }
    }
    while (m_runing.load() && d_ptr->audioDisplay->size() != 0) {
        msleep(s_waitQueueEmptyMilliseconds);
    }

    d_ptr->audioDisplay->stopDecoder();
}

} // namespace Ffmpeg
