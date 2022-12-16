#include "videodecoder.h"
#include "avcontextinfo.h"
#include "decodervideoframe.h"
#include "videoformat.hpp"

#include <QDebug>
#include <QPixmap>

namespace Ffmpeg {

class VideoDecoder::VideoDecoderPrivate
{
public:
    VideoDecoderPrivate(QObject *parent)
        : owner(parent)
    {
        decoderVideoFrame = new DecoderVideoFrame(owner);
    }

    QObject *owner;

    DecoderVideoFrame *decoderVideoFrame;
};

VideoDecoder::VideoDecoder(QObject *parent)
    : Decoder<Packet *>(parent)
    , d_ptr(new VideoDecoderPrivate(this))
{}

VideoDecoder::~VideoDecoder() {}

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
        if (m_seek) {
            clear();
            d_ptr->decoderVideoFrame->seek(m_seekTime, m_latchPtr.lock());
            seekFinish();
        }

        QScopedPointer<Packet> packetPtr(m_queue.dequeue());
        if (packetPtr.isNull()) {
            msleep(Sleep_Queue_Empty_Milliseconds);
            continue;
        }
        std::unique_ptr<Frame> framePtr(m_contextInfo->decodeFrame(packetPtr.data()));
        if (!framePtr) {
            continue;
        }
        Ffmpeg::calculateTime(framePtr.get(), m_contextInfo, m_formatContext);
        framePtr->setQImageFormat(
            VideoFormat::qFormatMaps.value(AVPixelFormat(framePtr->avFrame()->format),
                                           QImage::Format_Invalid));

        d_ptr->decoderVideoFrame->append(framePtr.release());

        while (m_runing && d_ptr->decoderVideoFrame->size() > Max_Frame_Size && !m_seek) {
            msleep(Sleep_Queue_Full_Milliseconds);
        }
    }
    while (m_runing && d_ptr->decoderVideoFrame->size() != 0) {
        msleep(Sleep_Queue_Full_Milliseconds);
    }
    d_ptr->decoderVideoFrame->stopDecoder();
}

} // namespace Ffmpeg
