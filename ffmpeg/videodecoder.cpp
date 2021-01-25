#include "playframe.h"
#include "videodecoder.h"
#include "avcontextinfo.h"
#include "codeccontext.h"
#include "avimage.h"
#include "formatcontext.h"
#include "audiodecoder.h"

#include <QDebug>
#include <QPixmap>

extern "C"{
#include <libavutil/time.h>
#include <libavdevice/avdevice.h>
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
        if(m_packetQueue.isEmpty()){
            msleep(1);
            continue;
        }

        Packet packet = m_packetQueue.takeFirst();

        if(!m_contextInfo->sendPacket(&packet)){
            continue;
        }
        if(!m_contextInfo->receiveFrame(&frame)){ // 一个packet一个视频帧
            continue;
        }

        avImage.scale(&frame, &frameRGB, m_contextInfo->codecCtx()->height());
        QPixmap pixmap(QPixmap::fromImage(frameRGB.toImage(m_contextInfo->codecCtx())));

        double duration = 0;
        double pts = 0;
        calculateTime(frame.avFrame(), duration, pts);

        double diff = (pts - AudioDecoder::audioClock()) * 1000;
        if(diff < 0.0)
            continue;
        msleep(diff);

        emit readyRead(pixmap); // 略慢于音频
    }
    m_contextInfo->clearImageBuffer();
}

void VideoDecoder::calculateTime(AVFrame *frame, double &duration, double &pts)
{
    AVRational tb = m_contextInfo->stream()->time_base;
    AVRational frame_rate = av_guess_frame_rate(m_formatContext->avFormatContext(), m_contextInfo->stream(), NULL);
    // 当前帧播放时长
    duration = (frame_rate.num && frame_rate.den ? av_q2d(AVRational{frame_rate.den, frame_rate.num}) : 0);
    // 当前帧显示时间戳
    pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
    //qDebug() << "video: " << duration << pts;
}

}
