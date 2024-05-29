#include "subtitledecoder.h"
#include "ffmpegutils.hpp"
#include "subtitle.h"
#include "subtitledisplay.hpp"

#include <event/seekevent.hpp>

extern "C" {
#include <libavcodec/packet.h>
}

namespace Ffmpeg {

class SubtitleDecoder::SubtitleDecoderPrivate
{
public:
    explicit SubtitleDecoderPrivate(SubtitleDecoder *q)
        : q_ptr(q)
    {
        decoderSubtitleFrame = new SubtitleDisplay(q_ptr);
    }

    void processEvent() const
    {
        while (q_ptr->m_runing.load() && !q_ptr->m_eventQueue.empty()) {
            auto eventPtr = q_ptr->m_eventQueue.take();
            switch (eventPtr->type()) {
            case Event::EventType::Pause: decoderSubtitleFrame->addEvent(eventPtr); break;
            case Event::EventType::Seek: {
                auto *seekEvent = static_cast<SeekEvent *>(eventPtr.data());
                seekEvent->countDown();
                q_ptr->clear();
                decoderSubtitleFrame->addEvent(eventPtr);
            } break;
            default: break;
            }
        }
    }

    SubtitleDecoder *q_ptr;

    SubtitleDisplay *decoderSubtitleFrame;
};

SubtitleDecoder::SubtitleDecoder(QObject *parent)
    : Decoder<PacketPtr>(parent)
    , d_ptr(new SubtitleDecoderPrivate(this))
{}

SubtitleDecoder::~SubtitleDecoder()
{
    stopDecoder();
}

void SubtitleDecoder::setVideoResolutionRatio(const QSize &size)
{
    d_ptr->decoderSubtitleFrame->setVideoResolutionRatio(size);
}

void SubtitleDecoder::setVideoRenders(const QVector<VideoRender *> &videoRenders)
{
    d_ptr->decoderSubtitleFrame->setVideoRenders(videoRenders);
}

void SubtitleDecoder::runDecoder()
{
    d_ptr->decoderSubtitleFrame->startDecoder(m_formatContext, m_contextInfo);

    while (m_runing) {
        d_ptr->processEvent();

        auto packetPtr(m_queue.take());
        if (packetPtr.isNull()) {
            continue;
        }
        //qDebug() << "packet ass :" << QString::fromUtf8(packetPtr->avPacket()->data);
        SubtitlePtr subtitlePtr(new Subtitle);
        if (!m_contextInfo->decodeSubtitle2(subtitlePtr, packetPtr)) {
            continue;
        }

        calculatePts(packetPtr.data(), m_contextInfo);
        subtitlePtr->setDefault(packetPtr->pts(),
                                packetPtr->duration(),
                                reinterpret_cast<const char *>(packetPtr->avPacket()->data));

        d_ptr->decoderSubtitleFrame->append(subtitlePtr);
    }
    while (m_runing && d_ptr->decoderSubtitleFrame->size() != 0) {
        msleep(s_waitQueueEmptyMilliseconds);
    }
    d_ptr->decoderSubtitleFrame->stopDecoder();
}

} // namespace Ffmpeg
