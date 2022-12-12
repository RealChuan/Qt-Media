#include "decodervideoframe.h"

#include <videooutput/videooutputrender.hpp>
#include <videorender/videorender.hpp>

#include <QDebug>
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
    QVector<VideoRender *> videoRenders;
};

DecoderVideoFrame::DecoderVideoFrame(QObject *parent)
    : Decoder<Frame *>(parent)
    , d_ptr(new DecoderVideoFramePrivate(this))
{}

DecoderVideoFrame::~DecoderVideoFrame() {}

void DecoderVideoFrame::stopDecoder()
{
    pause(false);
    Decoder<Frame *>::stopDecoder();
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

void DecoderVideoFrame::setVideoOutputRenders(QVector<VideoRender *> videoRenders)
{
    d_ptr->videoRenders = videoRenders;
}

void DecoderVideoFrame::runDecoder()
{
    for (auto render : d_ptr->videoRenders) {
        render->resetFps();
    }
    quint64 dropNum = 0;
    while (m_runing) {
        checkPause();
        checkSeek();

        QSharedPointer<Frame> framePtr(m_queue.dequeue());
        if (framePtr.isNull()) {
            msleep(Sleep_Queue_Empty_Milliseconds);
            continue;
        }
        double pts = framePtr->pts();
        if (m_seekTime > pts) {
            continue;
        }
        double diff = (pts - mediaClock()) * 1000;
        if (diff < Drop_Milliseconds || (mediaSpeed() > 1.0 && qAbs(diff) > UnWait_Milliseconds)) {
            dropNum++;
            continue;
        } else if (diff > UnWait_Milliseconds && !m_seek && !d_ptr->pause) {
            QMutexLocker locker(&d_ptr->mutex);
            d_ptr->waitCondition.wait(&d_ptr->mutex, diff);
        }
        // 略慢于音频
        for (auto render : d_ptr->videoRenders) {
            render->setFrame(framePtr);
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
