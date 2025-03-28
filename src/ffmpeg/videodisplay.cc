#include "videodisplay.hpp"
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

class VideoDisplay::VideoDisplayPrivate
{
public:
    explicit VideoDisplayPrivate(VideoDisplay *q)
        : q_ptr(q)
    {
        clock = new Clock(q);
    }

    void renderFrame(const QSharedPointer<Frame> &framePtr)
    {
        QMutexLocker locker(&mutex_render);
        for (auto *render : videoRenders) {
            render->setFrame(framePtr);
        }
    }

    void processEvent(bool &firstFrame) const
    {
        while (q_ptr->m_runing.load() && !q_ptr->m_eventQueue.isEmpty()) {
            qDebug() << "DecoderVideoFrame::processEvent";
            auto eventPtr = q_ptr->m_eventQueue.take();
            switch (eventPtr->type()) {
            case Event::EventType::Pause: {
                auto *pauseEvent = static_cast<PauseEvent *>(eventPtr.data());
                auto paused = pauseEvent->paused();
                clock->setPaused(paused);
            } break;
            case Event::EventType::Seek: {
                q_ptr->clear();
                firstFrame = false;
            }
            default: break;
            }
        }
    }

    VideoDisplay *q_ptr;

    Clock *clock;

    QMutex mutex;
    QWaitCondition waitCondition;

    QMutex mutex_render;
    QList<VideoRender *> videoRenders = {};
};

VideoDisplay::VideoDisplay(QObject *parent)
    : Decoder<FramePtr>(parent)
    , d_ptr(new VideoDisplayPrivate(this))
{}

VideoDisplay::~VideoDisplay()
{
    stopDecoder();
}

void VideoDisplay::setVideoRenders(const QList<VideoRender *> &videoRenders)
{
    QMutexLocker locker(&d_ptr->mutex_render);
    d_ptr->videoRenders = videoRenders;
}

void VideoDisplay::setMasterClock()
{
    Clock::setMaster(d_ptr->clock);
}

void VideoDisplay::runDecoder()
{
    for (auto *render : d_ptr->videoRenders) {
        render->resetFps();
    }
    quint64 dropNum = 0;
    bool firstFrame = false;
    while (m_runing.load()) {
        d_ptr->processEvent(firstFrame);

        auto framePtr(m_queue.take());
        if (framePtr.isNull()) {
            continue;
        }
        if (!firstFrame) {
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
        if (!d_ptr->clock->adjustDelay(delay)) {
            qDebug() << "Video Delay: " << delay;
            dropNum++;
            continue;
        }
        if (delay > 0) {
            QMutexLocker locker(&d_ptr->mutex);
            d_ptr->waitCondition.wait(&d_ptr->mutex, delay / 1000);
        }
        d_ptr->renderFrame(framePtr);
    }
    qInfo() << "Video Drop Num:" << dropNum;
}

} // namespace Ffmpeg
