#include "decodervideoframe.h"
#include "avcontextinfo.h"
#include "avimage.h"
#include "codeccontext.h"
#include "formatcontext.h"
#include "decoderaudioframe.h"

#include <QPixmap>
#include <QDebug>
#include <QWaitCondition>

extern "C"{
#include <libavutil/time.h>
#include <libavdevice/avdevice.h>
}

namespace Ffmpeg {

class DecoderVideoFramePrivate{
public:
    DecoderVideoFramePrivate(QObject *parent)
        : owner(parent){

    }

    QObject *owner;
    bool pause = false;
    QMutex mutex;
    QWaitCondition waitCondition;
};

DecoderVideoFrame::DecoderVideoFrame(QObject *parent)
    : Decoder<PlayFrame>(parent)
    , d_ptr(new DecoderVideoFramePrivate(this))
{

}

DecoderVideoFrame::~DecoderVideoFrame()
{
    stopDecoder();
}

void DecoderVideoFrame::stopDecoder()
{
    pause(false);
    Decoder<PlayFrame>::stopDecoder();
}

void DecoderVideoFrame::pause(bool state)
{
    d_ptr->pause = state;
    if(state)
        return;
    d_ptr->waitCondition.wakeOne();
}

void DecoderVideoFrame::runDecoder()
{
    PlayFrame frameRGB;

    m_contextInfo->imageBuffer(frameRGB);
    AVImage avImage(m_contextInfo->codecCtx());

    //int drop = 0;
    while(m_runing){
        checkPause();
        checkSeek();

        if(m_queue.isEmpty()){
            msleep(1);
            continue;
        }

        PlayFrame frame = m_queue.takeFirst();

        double duration = 0;
        double pts = 0;
        calculateTime(frame.avFrame(), duration, pts);

        if(m_seekTime - Seek_Error_Time > pts)
            continue;

        avImage.scale(&frame, &frameRGB, m_contextInfo->codecCtx()->height());
        QImage image(frameRGB.toImage(m_contextInfo->codecCtx()));

        double diff = (pts - DecoderAudioFrame::audioClock()) * 1000;
        if(diff <= 0.0){ // 暂时不丢帧，快速显示
            // drop++
        }else{
            msleep(diff);
        }
        emit readyRead(image); // 略慢于音频
    }
    QThread::sleep(1); // 最后一帧
    m_contextInfo->clearImageBuffer();
}

void DecoderVideoFrame::checkPause()
{
    while(d_ptr->pause){
        QMutexLocker locker(&d_ptr->mutex);
        d_ptr->waitCondition.wait(&d_ptr->mutex);
    }
}

void DecoderVideoFrame::checkSeek()
{
    if(!m_seek)
        return;

    seekCodec(m_seekTime);
    seekFinish();
}

}
