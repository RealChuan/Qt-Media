#include "videodecoder.h"
#include "avcontextinfo.h"
#include "decodervideoframe.h"

#include <QDebug>
#include <QPixmap>

extern "C"{
#include <libavutil/time.h>
#include <libavdevice/avdevice.h>
}

namespace Ffmpeg {

class VideoDecoderPrivate{
public:
    VideoDecoderPrivate(QObject *parent)
        : owner(parent){
        decoderVideoFrame = new DecoderVideoFrame(owner);
    }

    QObject *owner;

    DecoderVideoFrame *decoderVideoFrame;
};

VideoDecoder::VideoDecoder(QObject *parent)
    : Decoder<Packet>(parent)
    , d_ptr(new VideoDecoderPrivate(this))
{
    connect(d_ptr->decoderVideoFrame, &DecoderVideoFrame::readyRead, this, &VideoDecoder::readyRead);
}

VideoDecoder::~VideoDecoder()
{
    stopDecoder();
}

void VideoDecoder::pause(bool state)
{
    d_ptr->decoderVideoFrame->pause(state);
}

void VideoDecoder::clear()
{
    m_queue.clear();
    d_ptr->decoderVideoFrame->clear();
}

void VideoDecoder::runDecoder()
{
    PlayFrame frame;

    d_ptr->decoderVideoFrame->startDecoder(m_formatContext, m_contextInfo);

    while(m_runing){
        if(m_queue.isEmpty()){
            msleep(1);
            continue;
        }

        Packet packet = m_queue.takeFirst();

        if(!m_contextInfo->sendPacket(&packet)){
            continue;
        }
        if(!m_contextInfo->receiveFrame(&frame)){ // 一个packet一个视频帧
            continue;
        }

        d_ptr->decoderVideoFrame->append(frame);

        while(d_ptr->decoderVideoFrame->size() > 10)
            msleep(40);
    }

    while(m_runing && d_ptr->decoderVideoFrame->size() != 0)
        msleep(40);

    d_ptr->decoderVideoFrame->stopDecoder();
}

}
