#include "audiodisplay.hpp"
#include "clock.hpp"

#include <audiorender/audiooutputthread.hpp>
#include <event/seekevent.hpp>
#include <event/valueevent.hpp>

#include <QTime>
#include <QWaitCondition>

extern "C" {
#include <libavutil/time.h>
}

namespace Ffmpeg {

class AudioDisplay::AudioDisplayPrivate
{
public:
    explicit AudioDisplayPrivate(AudioDisplay *q)
        : q_ptr(q)
    {
        clock = new Clock(q);
    }

    ~AudioDisplayPrivate() = default;

    void processEvent(bool &firstFrame) const
    {
        while (q_ptr->m_runing.load() && !q_ptr->m_eventQueue.isEmpty()) {
            qDebug() << "AudioFramePrivate::processEvent";
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

    AudioDisplay *q_ptr;

    qreal volume = 0.5;
    QPointer<AudioOutputThread> audioOutputThreadPtr;

    Clock *clock;
    QMutex mutex;
    QWaitCondition waitCondition;
};

AudioDisplay::AudioDisplay(QObject *parent)
    : Decoder<FramePtr>(parent)
    , d_ptr(new AudioDisplayPrivate(this))
{}

AudioDisplay::~AudioDisplay()
{
    stopDecoder();
}

void AudioDisplay::setVolume(qreal volume)
{
    d_ptr->volume = volume;
    if (d_ptr->audioOutputThreadPtr != nullptr) {
        emit d_ptr->audioOutputThreadPtr->volumeChanged(d_ptr->volume);
    }
}

void AudioDisplay::setMasterClock()
{
    Clock::setMaster(d_ptr->clock);
}

void AudioDisplay::runDecoder()
{
    quint64 dropNum = 0;
    QScopedPointer<AudioOutputThread> audioOutputThreadPtr(new AudioOutputThread);
    d_ptr->audioOutputThreadPtr = audioOutputThreadPtr.data();
    audioOutputThreadPtr->openOutput(m_contextInfo, d_ptr->volume);
    bool firstFrame = false;
    while (m_runing.load()) {
        d_ptr->processEvent(firstFrame);

        auto framePtr(m_queue.take());
        if (nullptr == framePtr) {
            continue;
        }
        if (!firstFrame) {
            qDebug() << "Audio firstFrame: "
                     << QTime::fromMSecsSinceStartOfDay(framePtr->pts() / 1000)
                            .toString("hh:mm:ss.zzz");
            firstFrame = true;
            d_ptr->clock->reset(framePtr->pts());
        }
        emit audioOutputThreadPtr->convertData(framePtr);
        auto pts = framePtr->pts();
        d_ptr->clock->update(pts, av_gettime_relative());
        qint64 delay = 0;
        if (!d_ptr->clock->getDelayWithMaster(delay)) {
            continue;
        }
        auto emitPosition = qScopeGuard([this, pts]() { emit positionChanged(pts); });
        if (!d_ptr->clock->adjustDelay(delay)) {
            qDebug() << "Audio Delay: " << delay;
            dropNum++;
            continue;
        }
        delay /= 1000;
        if (delay > 0) {
            QMutexLocker locker(&d_ptr->mutex);
            d_ptr->waitCondition.wait(&d_ptr->mutex, delay);
        }
        // qDebug() << "Audio PTS:"
        //          << QTime::fromMSecsSinceStartOfDay(pts / 1000).toString("hh:mm:ss.zzz");
        emit audioOutputThreadPtr->wirteData();
    }
    qInfo() << "Audio Drop Num:" << dropNum;
}

} // namespace Ffmpeg
