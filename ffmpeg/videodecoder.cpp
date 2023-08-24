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
{}

VideoDecoder::~VideoDecoder() = default;

void VideoDecoder::seek(qint64 seekTime)
{
    Decoder<PacketPtr>::seek(seekTime);
    d_ptr->decoderVideoFrame->seek(seekTime);
}

void VideoDecoder::pause(bool state)
{
    d_ptr->decoderVideoFrame->pause(state);
}

void VideoDecoder::setVideoRenders(QVector<VideoRender *> videoRenders)
{
    d_ptr->decoderVideoFrame->setVideoRenders(videoRenders);
}

void VideoDecoder::runDecoder()
{
    d_ptr->decoderVideoFrame->startDecoder(m_formatContext, m_contextInfo);

    while (m_runing) {
        auto packetPtr(m_queue.take());
        if (packetPtr.isNull()) {
            continue;
        }
        auto frames(m_contextInfo->decodeFrame(packetPtr.data()));
        for (auto frame : frames) {
            Ffmpeg::calculateTime(frame, m_contextInfo, m_formatContext);
            d_ptr->decoderVideoFrame->append(FramePtr(frame));
        }
    }
    while (m_runing && d_ptr->decoderVideoFrame->size() != 0) {
        msleep(Sleep_Queue_Full_Milliseconds);
    }
    d_ptr->decoderVideoFrame->stopDecoder();
}

} // namespace Ffmpeg
