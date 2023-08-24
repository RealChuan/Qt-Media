#include "decodervideoframe.h"

#include <videorender/videorender.hpp>

#include <QDebug>
#include <QWaitCondition>

namespace Ffmpeg {

class DecoderVideoFrame::DecoderVideoFramePrivate
{
public:
    explicit DecoderVideoFramePrivate(DecoderVideoFrame *q)
        : q_ptr(q)
    {}

    DecoderVideoFrame *q_ptr;

    bool pause = false;
    QMutex mutex;
    QWaitCondition waitCondition;

    QMutex mutex_render;
    QVector<VideoRender *> videoRenders = {};
};

DecoderVideoFrame::DecoderVideoFrame(QObject *parent)
    : Decoder<FramePtr>(parent)
    , d_ptr(new DecoderVideoFramePrivate(this))
{}

DecoderVideoFrame::~DecoderVideoFrame() {}

void DecoderVideoFrame::stopDecoder()
{
    pause(false);
    Decoder<FramePtr>::stopDecoder();
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

void DecoderVideoFrame::setVideoRenders(QVector<VideoRender *> videoRenders)
{
    QMutexLocker locker(&d_ptr->mutex_render);
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

        auto framePtr(m_queue.take());
        if (framePtr.isNull()) {
            continue;
        }
        auto pts = framePtr->pts();
        if (m_seekTime > pts) {
            continue;
        }
        auto diff = pts - mediaClock();
        if (diff < Drop_Microseconds || (mediaSpeed() > 1.0 && qAbs(diff) > UnWait_Microseconds)) {
            dropNum++;
            continue;
        } else if (diff > UnWait_Microseconds && !d_ptr->pause) {
            QMutexLocker locker(&d_ptr->mutex);
            d_ptr->waitCondition.wait(&d_ptr->mutex, diff / 1000);
        }
        // 略慢于音频
        renderFrame(framePtr);
    }
    qInfo() << "Video Drop Num:" << dropNum;
}

void DecoderVideoFrame::checkPause()
{
    while (d_ptr->pause) {
        QMutexLocker locker(&d_ptr->mutex);
        d_ptr->waitCondition.wait(&d_ptr->mutex);
    }
}

void DecoderVideoFrame::renderFrame(const QSharedPointer<Frame> &framePtr)
{
    QMutexLocker locker(&d_ptr->mutex_render);
    for (auto render : d_ptr->videoRenders) {
        render->setFrame(framePtr);
    }
}

} // namespace Ffmpeg
