#include "decodervideoframe.h"
#include "clock.hpp"

#include <event/seekevent.hpp>
#include <event/valueevent.hpp>
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

    void processEvent(bool &firstFrame)
    {
        while (q_ptr->m_runing.load() && q_ptr->m_eventQueue.size() > 0) {
            qDebug() << "DecoderVideoFrame::processEvent";
            auto eventPtr = q_ptr->m_eventQueue.take();
            switch (eventPtr->type()) {
            case Event::EventType::Pause: {
                auto pauseEvent = static_cast<PauseEvent *>(eventPtr.data());
                auto paused = pauseEvent->paused();
                clock->setPaused(paused);
            } break;
            case Event::EventType::Seek: {
                auto seekEvent = static_cast<SeekEvent *>(eventPtr.data());
                auto position = seekEvent->position();
                q_ptr->clear();
                firstFrame = false;
            }
            default: break;
            }
        }
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

DecoderVideoFrame::~DecoderVideoFrame()
{
    stopDecoder();
}

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
        d_ptr->processEvent(firstFrame);

        auto framePtr(m_queue.take());
        if (framePtr.isNull()) {
            continue;
        } else if (!firstFrame) {
            qDebug() << "Video firstFrame: "
                     << QTime::fromMSecsSinceStartOfDay(framePtr->pts() / 1000)
                            .toString("hh:mm:ss.zzz");
            firstFrame = true;
            d_ptr->clock->reset(framePtr->pts());
        }
        auto pts = framePtr->pts();
        d_ptr->clock->update(pts, av_gettime_relative());
        qint64 delay = 0;
        if (!d_ptr->clock->getDelayWithMaster(delay)) {
            continue;
        }
        auto emitPosition = qScopeGuard([=]() { emit positionChanged(pts); });
        if (!Clock::adjustDelay(delay)) {
            qDebug() << "Video Delay: " << delay;
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
