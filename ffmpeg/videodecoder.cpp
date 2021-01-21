#include "playframe.h"
#include "videodecoder.h"
#include "avcontextinfo.h"
#include "codeccontext.h"
#include "avimage.h"
#include "packet.h"

#include <utils/taskqueue.h>

#include <QPixmap>

namespace Ffmpeg {

class VideoDecoderPrivate{
public:
    VideoDecoderPrivate(QObject *parent)
        : owner(parent){

    }
    QObject *owner;

    Utils::Queue<Packet> packetQueue;
    AVContextInfo *videoInfo;

    bool runing = true;
};

VideoDecoder::VideoDecoder(QObject *parent)
    : QThread(parent)
    , d_ptr(new VideoDecoderPrivate(this))
{

}

VideoDecoder::~VideoDecoder()
{
    stopDecoder();
}

void VideoDecoder::startDecoder(AVContextInfo *audioInfo)
{
    stopDecoder();
    d_ptr->videoInfo = audioInfo;
    d_ptr->runing = true;
    start();
}

void VideoDecoder::stopDecoder()
{
    d_ptr->runing = false;
    if(isRunning()){
        quit();
        wait();
    }
}

void VideoDecoder::append(const Packet &packet)
{
    d_ptr->packetQueue.append(packet);
}

int VideoDecoder::size()
{
    return d_ptr->packetQueue.size();
}

void VideoDecoder::run()
{
    Q_ASSERT(d_ptr->videoInfo != nullptr);

    PlayFrame frame;
    PlayFrame frameRGB;

    d_ptr->videoInfo->imageBuffer(frameRGB);
    AVImage avImage(d_ptr->videoInfo->codecCtx());

    while(d_ptr->runing){
        msleep(20);

        if(d_ptr->packetQueue.isEmpty())
            continue;

        Packet packet = d_ptr->packetQueue.takeFirst();

        if(!d_ptr->videoInfo->sendPacket(&packet)){
            continue;
        }
        if(!d_ptr->videoInfo->receiveFrame(&frame)){
            continue;
        }

        avImage.scale(&frame, &frameRGB, d_ptr->videoInfo->codecCtx()->height());

        emit readyRead(QPixmap::fromImage(frameRGB.toImage(d_ptr->videoInfo->codecCtx()->width(),
                                                           d_ptr->videoInfo->codecCtx()->height())));
        packet.clear();
    }
    d_ptr->videoInfo->clearImageBuffer();
}


}
