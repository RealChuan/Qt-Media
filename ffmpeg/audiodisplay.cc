#include "audiodisplay.hpp"
#include "audioframeconverter.h"
#include "clock.hpp"
#include "codeccontext.h"

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

    void processEvent(bool &firstFrame)
    {
        while (q_ptr->m_runing.load() && q_ptr->m_eventQueue.size() > 0) {
            qDebug() << "AudioFramePrivate::processEvent";
            auto eventPtr = q_ptr->m_eventQueue.take();
            switch (eventPtr->type()) {
            case Event::EventType::Pause: {
                auto pauseEvent = static_cast<PauseEvent *>(eventPtr.data());
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
    if (d_ptr->audioOutputThreadPtr) {
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
    int sampleSize = 0;
    auto format = getAudioFormatFromCodecCtx(m_contextInfo->codecCtx(), sampleSize);
    QScopedPointer<AudioOutputThread> audioOutputThreadPtr(new AudioOutputThread);
    d_ptr->audioOutputThreadPtr = audioOutputThreadPtr.data();
    audioOutputThreadPtr->openOutput({format, format.sampleRate() * sampleSize / 8, d_ptr->volume});
    AudioFrameConverter audioConverter(m_contextInfo->codecCtx(), format);
    bool firstFrame = false;
    while (m_runing.load()) {
        d_ptr->processEvent(firstFrame);

        auto framePtr(m_queue.take());
        if (framePtr.isNull()) {
            continue;
        } else if (!firstFrame) {
            qDebug() << "Audio firstFrame: "
                     << QTime::fromMSecsSinceStartOfDay(framePtr->pts() / 1000)
                            .toString("hh:mm:ss.zzz");
            firstFrame = true;
            d_ptr->clock->reset(framePtr->pts());
        }
        auto audioBuf = audioConverter.convert(framePtr.data());
        auto pts = framePtr->pts();
        d_ptr->clock->update(pts, av_gettime_relative());
        qint64 delay = 0;
        if (!d_ptr->clock->getDelayWithMaster(delay)) {
            continue;
        }
        auto emitPosition = qScopeGuard([=]() { emit positionChanged(pts); });
        if (!Clock::adjustDelay(delay)) {
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
        emit audioOutputThreadPtr->wirteData(audioBuf);
    }
    qInfo() << "Audio Drop Num:" << dropNum;
}

} // namespace Ffmpeg
