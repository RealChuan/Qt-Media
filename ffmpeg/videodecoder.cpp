#include "playframe.h"
#include "videodecoder.h"
#include "avcontextinfo.h"
#include "codeccontext.h"
#include "avimage.h"
#include "formatcontext.h"

#include <QDebug>
#include <QPixmap>

extern "C"{
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

VideoDecoder::VideoDecoder(QObject *parent)
    : Decoder(parent)
{

}

void VideoDecoder::runDecoder()
{
    PlayFrame frame;
    PlayFrame frameRGB;

    m_contextInfo->imageBuffer(frameRGB);
    AVImage avImage(m_contextInfo->codecCtx());

    while(m_runing){
        msleep(100);

        if(m_packetQueue.isEmpty())
            continue;

        Packet packet = m_packetQueue.takeFirst();

        if(!m_contextInfo->sendPacket(&packet)){
            continue;
        }
        if(!m_contextInfo->receiveFrame(&frame)){
            continue;
        }

        double duration = 0;
        double pts = 0;
        calculateTime(frame, duration, pts);

        avImage.scale(&frame, &frameRGB, m_contextInfo->codecCtx()->height());

        emit readyRead(QPixmap::fromImage(frameRGB.toImage(m_contextInfo->codecCtx()->width(),
                                                           m_contextInfo->codecCtx()->height())));
        packet.clear();
    }
    m_contextInfo->clearImageBuffer();
}

void VideoDecoder::calculateTime(PlayFrame &frame, double &duration, double &pts)
{
    AVRational tb = m_contextInfo->stream()->time_base;
    AVRational frame_rate = av_guess_frame_rate(m_formatContext->avFormatContext(), m_contextInfo->stream(), NULL);
    // 当前帧播放时长
    duration = (frame_rate.num && frame_rate.den ? av_q2d(AVRational{frame_rate.den, frame_rate.num}) : 0);
    // 当前帧显示时间戳
    pts = (frame.avFrame()->pts == AV_NOPTS_VALUE) ? NAN : frame.avFrame()->pts * av_q2d(tb);
    qDebug() << "video: " << duration << pts;
}

}
