#include "decodervideoframe.h"
#include "clock.hpp"

#include <videorender/videorender.hpp>

#include <QDebug>
#include <QTime>
#include <QWaitCondition>

extern "C" {
#include <libavutil/time.h>
}

namespace Ffmpeg {

class DecoderVideoFrame::DecoderVideoFramePrivate
{
public:
    explicit DecoderVideoFramePrivate(DecoderVideoFrame *q)
        : q_ptr(q)
    {
        clock = new Clock(q);
    }

    DecoderVideoFrame *q_ptr;

    Clock *clock;

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

void DecoderVideoFrame::setVideoRenders(QVector<VideoRender *> videoRenders)
{
    QMutexLocker locker(&d_ptr->mutex_render);
    d_ptr->videoRenders = videoRenders;
}

void DecoderVideoFrame::setMasterClock()
{
    Clock::setMaster(d_ptr->clock);
}

void DecoderVideoFrame::runDecoder()
{
    for (auto render : d_ptr->videoRenders) {
        render->resetFps();
    }
    quint64 dropNum = 0;
    bool firstFrame = false;
    while (m_runing.load()) {
        auto framePtr(m_queue.take());
        if (framePtr.isNull()) {
            continue;
        } else if (!firstFrame) {
            firstFrame = true;
            d_ptr->clock->reset(framePtr->pts());
        }
        auto pts = framePtr->pts();
        auto emitPosition = qScopeGuard([=]() { emit positionChanged(pts); });
        d_ptr->clock->update(pts, av_gettime_relative());
        qint64 delay = 0;
        if (!d_ptr->clock->getDelayWithMaster(delay)) {
            continue;
        }
        if (!Clock::adjustDelay(delay)) {
            dropNum++;
            continue;
        }
        if (delay > 0) {
            QMutexLocker locker(&d_ptr->mutex);
            d_ptr->waitCondition.wait(&d_ptr->mutex, delay / 1000);
        }
        // qDebug() << "Video PTS:"
        //          << QTime::fromMSecsSinceStartOfDay(pts / 1000).toString("hh:mm:ss.zzz");

        renderFrame(framePtr);
    }
    qInfo() << "Video Drop Num:" << dropNum;
}

void DecoderVideoFrame::renderFrame(const QSharedPointer<Frame> &framePtr)
{
    QMutexLocker locker(&d_ptr->mutex_render);
    for (auto render : d_ptr->videoRenders) {
        render->setFrame(framePtr);
    }
}

} // namespace Ffmpeg
