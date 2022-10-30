#include "decodervideoframe.h"
#include "avcontextinfo.h"
#include "avimage.h"
#include "codeccontext.h"

#include <videooutput/videooutputrender.hpp>

#include <QDebug>
#include <QPixmap>
#include <QWaitCondition>

namespace Ffmpeg {

class DecoderVideoFrame::DecoderVideoFramePrivate
{
public:
    DecoderVideoFramePrivate(QObject *parent)
        : owner(parent)
    {}

    QObject *owner;
    bool pause = false;
    QMutex mutex;
    QWaitCondition waitCondition;

    QVector<VideoOutputRender *> videoOutputRenders;
};

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
    if (!isRunning()) {
        return;
    }
    d_ptr->pause = state;
    if (state) {
        return;
    }
    d_ptr->waitCondition.wakeOne();
}

void DecoderVideoFrame::setVideoOutputRenders(QVector<VideoOutputRender *> videoOutputRenders)
{
    d_ptr->videoOutputRenders = videoOutputRenders;
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
            msleep(Sleep_Queue_Empty_Milliseconds);
            continue;
        }

        QScopedPointer<PlayFrame> framePtr(m_queue.dequeue());
        double duration = 0;
        double pts = 0;
        calculateTime(framePtr->avFrame(), duration, pts);
        if (m_seekTime > pts) {
            continue;
        }

#ifdef HardWareDecodeOn
        bool ok = false;
        framePtr.reset(m_contextInfo->hardWareDecode()->transforFrame(framePtr.get(), ok));
        //qDebug() << framePtr->avFrame()->format;
#endif
        avImage.scale(framePtr.data(), &frameRGB, m_contextInfo->codecCtx()->height());
        QImage image(frameRGB.toImage(m_contextInfo->codecCtx()));

        double diff = (pts - clock()) * 1000;
        if (diff <= 0) {
            dropNum++;
            continue;
        } else if (diff > UnWait_Milliseconds && !m_seek && !d_ptr->pause) {
            QMutexLocker locker(&d_ptr->mutex);
            d_ptr->waitCondition.wait(&d_ptr->mutex, diff);
            //msleep(diff);
        }
        // 略慢于音频
        for (auto render : d_ptr->videoOutputRenders) {
            render->setDisplayImage(image);
        }
    }
    qInfo() << dropNum;
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
    if (!m_seek) {
        return;
    }
    clear();
    auto latchPtr = m_latchPtr.lock();
    if (latchPtr) {
        latchPtr->countDown();
    }
    seekFinish();
}

} // namespace Ffmpeg
