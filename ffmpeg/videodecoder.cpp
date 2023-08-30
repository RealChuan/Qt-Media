#include "videodecoder.h"
#include "avcontextinfo.h"
#include "decodervideoframe.h"
#include "videoformat.hpp"

#include <QDebug>

namespace Ffmpeg {

class VideoDecoder::VideoDecoderPrivate
{
public:
    VideoDecoderPrivate(VideoDecoder *q)
        : q_ptr(q)
    {
        decoderVideoFrame = new DecoderVideoFrame(q_ptr);
    }

    VideoDecoder *q_ptr;

    DecoderVideoFrame *decoderVideoFrame;
};

VideoDecoder::VideoDecoder(QObject *parent)
    : Decoder<PacketPtr>(parent)
    , d_ptr(new VideoDecoderPrivate(this))
{
    connect(d_ptr->decoderVideoFrame,
            &DecoderVideoFrame::positionChanged,
            this,
            &VideoDecoder::positionChanged);
}

VideoDecoder::~VideoDecoder() = default;

void VideoDecoder::setVideoRenders(QVector<VideoRender *> videoRenders)
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
        auto packetPtr(m_queue.take());
        if (packetPtr.isNull()) {
            continue;
        }
        auto framePtrs = m_contextInfo->decodeFrame(packetPtr);
        for (const auto &framePtr : framePtrs) {
            Ffmpeg::calculatePts(framePtr.data(), m_contextInfo, m_formatContext);
            d_ptr->decoderVideoFrame->append(framePtr);
        }
    }
    while (m_runing && d_ptr->decoderVideoFrame->size() != 0) {
        msleep(Sleep_Queue_Full_Milliseconds);
    }
    d_ptr->decoderVideoFrame->stopDecoder();
}

} // namespace Ffmpeg
