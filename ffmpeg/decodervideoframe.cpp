#include "decodervideoframe.h"
#include "avcontextinfo.h"
#include "avimage.h"
#include "codeccontext.h"
#include "formatcontext.h"
#include "decoderaudioframe.h"

#include <QPixmap>
#include <QDebug>
#include <QWaitCondition>

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

}

void DecoderVideoFrame::stopDecoder()
{
    pause(false);
    Decoder<PlayFrame>::stopDecoder();
}

void DecoderVideoFrame::pause(bool state)
{
    if(!isRunning())
        return;

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

    qint64 dropNum = 0;
    qint64 playNum = 0;
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

        if(m_seekTime > pts)
            continue;

        avImage.scale(&frame, &frameRGB, m_contextInfo->codecCtx()->height());
        QImage image(frameRGB.toImage(m_contextInfo->codecCtx()));

        double diff = (pts - DecoderAudioFrame::audioClock()) * 1000;
        if(diff > 0){
            playNum++;
            msleep(diff);
        }else{
            dropNum++;
        }
        //基于信号槽的队列不可控，会产生堆积，不如自己建生成消费队列？
        emit readyRead(image); // 略慢于音频
    }
    m_contextInfo->clearImageBuffer();
    qDebug() << dropNum << playNum;
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
