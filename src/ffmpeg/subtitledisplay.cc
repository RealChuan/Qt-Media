#include "subtitledisplay.hpp"
#include "clock.hpp"
#include "codeccontext.h"

#include <event/seekevent.hpp>
#include <event/valueevent.hpp>
#include <subtitle/ass.hpp>
#include <videorender/videorender.hpp>

#include <QDebug>
#include <QImage>
#include <QWaitCondition>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}

namespace Ffmpeg {

class SubtitleDisplay::SubtitleDisplayPrivate
{
public:
    explicit SubtitleDisplayPrivate(SubtitleDisplay *q)
        : q_ptr(q)
    {
        clock = new Clock(q);
    }

    void renderFrame(const QSharedPointer<Subtitle> &subtitlePtr)
    {
        QMutexLocker locker(&mutex_render);
        for (auto *render : videoRenders) {
            render->setSubTitleFrame(subtitlePtr);
        }
    }

    void processEvent(Ass *ass, bool &firstFrame) const
    {
        while (q_ptr->m_runing.load() && !q_ptr->m_eventQueue.isEmpty()) {
            qDebug() << "DecoderSubtitleFrame::processEvent";
            auto eventPtr = q_ptr->m_eventQueue.take();
            switch (eventPtr->type()) {
            case Event::EventType::Pause: {
                auto *pauseEvent = static_cast<PauseEvent *>(eventPtr.data());
                auto paused = pauseEvent->paused();
                clock->setPaused(paused);
            } break;
            case Event::EventType::Seek: {
                q_ptr->clear();
                ass->flushASSEvents();
                firstFrame = false;
            }
            default: break;
            }
        }
    }

    SubtitleDisplay *q_ptr;

    Clock *clock;

    QMutex mutex;
    QWaitCondition waitCondition;
    QSize videoResolutionRatio = QSize(1280, 720);

    QMutex mutex_render;
    QList<VideoRender *> videoRenders = {};
};

SubtitleDisplay::SubtitleDisplay(QObject *parent)
    : Decoder<SubtitlePtr>(parent)
    , d_ptr(new SubtitleDisplayPrivate(this))
{}

SubtitleDisplay::~SubtitleDisplay()
{
    stopDecoder();
}

void SubtitleDisplay::setVideoResolutionRatio(const QSize &size)
{
    if (!size.isValid()) {
        return;
    }
    d_ptr->videoResolutionRatio = size;
}

void SubtitleDisplay::setVideoRenders(const QList<VideoRender *> &videoRenders)
{
    QMutexLocker locker(&d_ptr->mutex_render);
    d_ptr->videoRenders = videoRenders;
}

void SubtitleDisplay::runDecoder()
{
    quint64 dropNum = 0;
    auto *ctx = m_contextInfo->codecCtx()->avCodecCtx();
    QScopedPointer<Ass> assPtr(new Ass);
    if (ctx->subtitle_header != nullptr) {
        assPtr->init(ctx->subtitle_header, ctx->subtitle_header_size);
    }
    assPtr->setWindowSize(d_ptr->videoResolutionRatio);
    SwsContext *swsContext = nullptr;
    bool firstFrame = false;
    while (m_runing.load()) {
        d_ptr->processEvent(assPtr.data(), firstFrame);

        auto subtitlePtr(m_queue.take());
        if (subtitlePtr.isNull()) {
            continue;
        }
        if (!firstFrame) {
            qDebug() << "Subtitle firstFrame: "
                     << QTime::fromMSecsSinceStartOfDay(subtitlePtr->pts() / 1000)
                            .toString("hh:mm:ss.zzz");
            firstFrame = true;
            d_ptr->clock->reset(subtitlePtr->pts());
        }
        subtitlePtr->setVideoResolutionRatio(d_ptr->videoResolutionRatio);
        subtitlePtr->parse(&swsContext);
        if (subtitlePtr->type() == Subtitle::Type::ASS) {
            subtitlePtr->resolveAss(assPtr.data());
            subtitlePtr->generateImage();
        }
        d_ptr->clock->update(subtitlePtr->pts(), av_gettime_relative());
        qint64 delay = 0;
        if (!d_ptr->clock->getDelayWithMaster(delay)) {
            continue;
        }
        auto delayDuration = delay + subtitlePtr->duration();
        if (!d_ptr->clock->adjustDelay(delayDuration)) {
            qDebug() << "Subtitle Delay: " << delay;
            dropNum++;
            continue;
        }
        if (d_ptr->clock->adjustDelay(delay)) {
            if (delay > 0) {
                QMutexLocker locker(&d_ptr->mutex);
                d_ptr->waitCondition.wait(&d_ptr->mutex, delay / 1000);
            }
        }
        d_ptr->renderFrame(subtitlePtr);
    }
    sws_freeContext(swsContext);
    qInfo() << "Subtitle Drop Num:" << dropNum;
}

} // namespace Ffmpeg
