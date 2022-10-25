#include "videodecoder.h"
#include "avcontextinfo.h"
#include "decodervideoframe.h"

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

void VideoDecoder::setSpeed(double speed)
{
    Decoder<Packet *>::setSpeed(speed);
    d_ptr->decoderVideoFrame->setSpeed(speed);
}

void VideoDecoder::setVideoOutputRenders(QVector<VideoOutputRender *> videoOutputRenders)
{
    d_ptr->decoderVideoFrame->setVideoOutputRenders(videoOutputRenders);
}

void VideoDecoder::runDecoder()
{
    d_ptr->decoderVideoFrame->startDecoder(m_formatContext, m_contextInfo);

    while (m_runing) {
        if (m_seek) {
            d_ptr->decoderVideoFrame->seek(m_seekTime);
            seekFinish();
        }

        if (m_queue.isEmpty()) {
            msleep(Sleep_Milliseconds);
            continue;
        }

        QScopedPointer<Packet> packetPtr(m_queue.dequeue());

        if (!m_contextInfo->sendPacket(packetPtr.data())) {
            continue;
        }

        std::unique_ptr<PlayFrame> framePtr(new PlayFrame);
        if (!m_contextInfo->receiveFrame(framePtr.get())) { // 一个packet一个视频帧
            continue;
        }

        d_ptr->decoderVideoFrame->append(framePtr.release());

        while (m_runing && d_ptr->decoderVideoFrame->size() > Max_Frame_Size && !m_seek) {
            msleep(Sleep_Milliseconds);
        }
    }
    while (m_runing && d_ptr->decoderVideoFrame->size() != 0) {
        msleep(Sleep_Milliseconds);
    }
    d_ptr->decoderVideoFrame->stopDecoder();
}

} // namespace Ffmpeg
