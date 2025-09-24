#include "videodecoder.h"
#include "avcontextinfo.h"
#include "ffmpegutils.hpp"
#include "videodisplay.hpp"
#include "videoformat.hpp"

#include <event/seekevent.hpp>

#include <QDebug>

namespace Ffmpeg {

class VideoDecoder::VideoDecoderPrivate
{
public:
    explicit VideoDecoderPrivate(VideoDecoder *q)
        : q_ptr(q)
    {
        decoderVideoFrame = new VideoDisplay(q_ptr);
    }

    void processEvent() const
    {
        while (q_ptr->m_runing.load() && !q_ptr->m_eventQueue.isEmpty()) {
            auto eventPtr = q_ptr->m_eventQueue.take();
            switch (eventPtr->type()) {
            case Event::EventType::Pause: decoderVideoFrame->addEvent(eventPtr); break;
            case Event::EventType::Seek: {
                auto *seekEvent = static_cast<SeekEvent *>(eventPtr.data());
                seekEvent->countDown();
                q_ptr->clear();
                decoderVideoFrame->addEvent(eventPtr);
            } break;
            default: break;
            }
        }
    }

    VideoDecoder *q_ptr;

    VideoDisplay *decoderVideoFrame;
};

VideoDecoder::VideoDecoder(QObject *parent)
    : Decoder<PacketPtr>(parent)
    , d_ptr(new VideoDecoderPrivate(this))
{
    connect(d_ptr->decoderVideoFrame,
            &VideoDisplay::positionChanged,
            this,
            &VideoDecoder::positionChanged);
}

VideoDecoder::~VideoDecoder()
{
    stopDecoder();
}

void VideoDecoder::setVideoRenders(const QList<VideoRender *> &videoRenders)
{
    d_ptr->decoderVideoFrame->setVideoRenders(videoRenders);
}

void VideoDecoder::setMasterClock()
{
    d_ptr->decoderVideoFrame->setMasterClock();
}

void VideoDecoder::runDecoder()
{
    d_ptr->decoderVideoFrame->startDecoder(m_formatContext, m_contextInfo);

    while (m_runing) {
        d_ptr->processEvent();

        auto packetPtr(m_queue.take());
        if (packetPtr.isNull()) {
            continue;
        }
        auto framePtrs = m_contextInfo->decodeFrame(packetPtr);
        for (const auto &framePtr : std::as_const(framePtrs)) {
            calculatePts(framePtr, m_contextInfo, m_formatContext);
            d_ptr->decoderVideoFrame->append(framePtr);
        }
    }
    while (m_runing && d_ptr->decoderVideoFrame->size() != 0) {
        msleep(s_waitQueueEmptyMilliseconds);
    }
    d_ptr->decoderVideoFrame->stopDecoder();
}

} // namespace Ffmpeg
