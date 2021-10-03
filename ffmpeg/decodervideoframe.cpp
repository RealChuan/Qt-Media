#include "decodervideoframe.h"
#include "avcontextinfo.h"
#include "avimage.h"
#include "codeccontext.h"
#include "decoderaudioframe.h"
#include "formatcontext.h"

#include <QDebug>
#include <QPixmap>
#include <QWaitCondition>

namespace Ffmpeg {

class DecoderVideoFramePrivate
{
public:
    DecoderVideoFramePrivate(QObject *parent)
        : owner(parent)
    {}

    QObject *owner;
    bool pause = false;
    QMutex mutex;
    QWaitCondition waitCondition;
};

Utils::Queue<VideoFrame> DecoderVideoFrame::videoFrameQueue;

DecoderVideoFrame::DecoderVideoFrame(QObject *parent)
    : Decoder<PlayFrame *>(parent)
    , d_ptr(new DecoderVideoFramePrivate(this))
{}

DecoderVideoFrame::~DecoderVideoFrame() {}

void DecoderVideoFrame::stopDecoder()
{
    pause(false);
    Decoder<PlayFrame *>::stopDecoder();
}

void DecoderVideoFrame::pause(bool state)
{
    if (!isRunning())
        return;

    d_ptr->pause = state;
    if (state)
        return;
    d_ptr->waitCondition.wakeOne();
}

void DecoderVideoFrame::runDecoder()
{
    PlayFrame frameRGB;

    m_contextInfo->imageAlloc(frameRGB);
    AVImage avImage(m_contextInfo->codecCtx());

    quint64 dropNum = 0;
    while (m_runing) {
        checkPause();
        checkSeek();

        if (m_queue.isEmpty()) {
            msleep(1);
            continue;
        }
        QScopedPointer<PlayFrame> framePtr(m_queue.dequeue());

        double duration = 0;
        double pts = 0;
        calculateTime(framePtr->avFrame(), duration, pts);

        if (m_seekTime > pts)
            continue;

        avImage.scale(framePtr.data(), &frameRGB, m_contextInfo->codecCtx()->height());
        QImage image(frameRGB.toImage(m_contextInfo->codecCtx()));

        double diff = (pts - DecoderAudioFrame::audioClock()) * 1000;
        if (diff <= 0) {
            dropNum++;
            continue;
        } else {
            msleep(diff);
        }

        //基于信号槽的队列不可控，会产生堆积，不如自己建生成消费队列？
        emit readyRead(image); // 略慢于音频
        // 消息队列播放一卡一卡的
        //videoFrameQueue.enqueue(VideoFrame{pts, image});
    }
    qDebug() << dropNum;
}

void DecoderVideoFrame::checkPause()
{
    while (d_ptr->pause) {
        QMutexLocker locker(&d_ptr->mutex);
        d_ptr->waitCondition.wait(&d_ptr->mutex);
    }
}

void DecoderVideoFrame::checkSeek()
{
    if (!m_seek)
        return;

    seekCodec(m_seekTime);
    seekFinish();
}

} // namespace Ffmpeg
