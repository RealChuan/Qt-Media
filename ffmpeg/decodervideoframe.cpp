#include "decodervideoframe.h"
#include "avcontextinfo.h"
#include "avimage.h"
#include "codeccontext.h"
#include "formatcontext.h"
#include "decoderaudioframe.h"

#include <QPixmap>
#include <QDebug>

extern "C"{
#include <libavutil/time.h>
#include <libavdevice/avdevice.h>
}

namespace Ffmpeg {

DecoderVideoFrame::DecoderVideoFrame(QObject *parent)
    : Decoder(parent)
{

}

void DecoderVideoFrame::runDecoder()
{
    PlayFrame frameRGB;

    m_contextInfo->imageBuffer(frameRGB);
    AVImage avImage(m_contextInfo->codecCtx());

    //int drop = 0;
    while(m_runing){
        if(m_queue.isEmpty()){
            msleep(1);
            continue;
        }

        PlayFrame frame = m_queue.takeFirst();

        avImage.scale(&frame, &frameRGB, m_contextInfo->codecCtx()->height());
        QPixmap pixmap(QPixmap::fromImage(frameRGB.toImage(m_contextInfo->codecCtx())));

        double duration = 0;
        double pts = 0;
        calculateTime(frame.avFrame(), duration, pts);

        double diff = (pts - DecoderAudioFrame::audioClock()) * 1000;
        if(diff <= 0.0){
            // 暂时不丢帧，快速显示
            //qWarning() << "Drop frame: " << ++drop << diff;
            //continue;
        }else{
            //qInfo() << "Show frame: " << diff;
            msleep(diff);
        }
        emit readyRead(pixmap); // 略慢于音频
    }
    m_contextInfo->clearImageBuffer();
}

}
