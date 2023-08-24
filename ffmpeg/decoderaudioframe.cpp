#include "decoderaudioframe.h"
#include "audioframeconverter.h"
#include "codeccontext.h"

#include <QAudioDevice>
#include <QAudioSink>
#include <QCoreApplication>
#include <QMediaDevices>
#include <QMediaFormat>
#include <QTime>
#include <QWaitCondition>

namespace Ffmpeg {

class DecoderAudioFrame::DecoderAudioFramePrivate
{
public:
    explicit DecoderAudioFramePrivate(DecoderAudioFrame *q)
        : q_ptr(q)
    {}
    ~DecoderAudioFramePrivate() = default;

    DecoderAudioFrame *q_ptr;

    QScopedPointer<QAudioSink> audioSinkPtr;
    QIODevice *audioDevice = nullptr;
    qreal volume = 0.5;

    std::atomic_bool pause = false;
    double speed = 0;
    qint64 pauseTime;
    QMutex mutex;
    QWaitCondition waitCondition;

    qint64 seekTime = 0; // milliseconds
    std::atomic_bool firstSeekFrame = false;

    QScopedPointer<QMediaDevices> mediaDevicesPtr;
    std::atomic_bool audioOutputsChanged = false;
    std::atomic_bool isLocalFile = true;
};

DecoderAudioFrame::DecoderAudioFrame(QObject *parent)
    : Decoder<FramePtr>(parent)
    , d_ptr(new DecoderAudioFramePrivate(this))
{
    buildConnect();
}

DecoderAudioFrame::~DecoderAudioFrame() = default;

void DecoderAudioFrame::stopDecoder()
{
    pause(false);
    Decoder<FramePtr>::stopDecoder();
}

bool DecoderAudioFrame::seek(qint64 seekTime)
{
    if (!Decoder<FramePtr>::seek(seekTime)) {
        return false;
    }
    setMediaClock(m_seekTime);
    d_ptr->firstSeekFrame = true;
    d_ptr->pauseTime = 0;
    return true;
}

void DecoderAudioFrame::pause(bool state)
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

bool DecoderAudioFrame::isPause()
{
    return d_ptr->pause;
}

void DecoderAudioFrame::setVolume(qreal volume)
{
    d_ptr->volume = volume;
    if (d_ptr->audioSinkPtr) {
        d_ptr->audioSinkPtr->setVolume(volume);
    }
}

void DecoderAudioFrame::setIsLocalFile(bool isLocalFile)
{
    d_ptr->isLocalFile = isLocalFile;
}

void DecoderAudioFrame::onStateChanged(QAudio::State state)
{
    Q_UNUSED(state)
    if (d_ptr->audioSinkPtr.isNull()) {
        return;
    }
    if (d_ptr->audioSinkPtr->error() != QAudio::NoError) {
        qWarning() << tr("QAudioSink Error:") << state << d_ptr->audioSinkPtr->error();
    }
    //    switch (state) {
    //    case QAudio::StoppedState:
    //        // Stopped for other reasons
    //        if (d_ptr->audioSink->error() != QAudio::NoError) {
    //            qWarning() << tr("QAudioSink Error:") << d_ptr->audioSink->error();
    //        }
    //        break;
    //    default:
    //        // ... other cases as appropriate
    //        break;
    //    }
}

void DecoderAudioFrame::onAudioOutputsChanged()
{
    d_ptr->audioOutputsChanged = true;
}

void DecoderAudioFrame::runDecoder()
{
    quint64 dropNum = 0;
    QAudioDevice audioDevice(QMediaDevices::defaultAudioOutput());
    auto format = resetAudioOutput();
    d_ptr->seekTime = 0;
    d_ptr->pauseTime = 0;
    AudioFrameConverter audioConverter(m_contextInfo->codecCtx(), format);
    QElapsedTimer timer;
    if (d_ptr->isLocalFile) { // 音频播放依赖外部时钟，适用于本地文件播放
        while (m_runing) {
            checkDefaultAudioOutput(audioDevice);
            checkPause(d_ptr->pauseTime);
            checkSpeed(timer, d_ptr->pauseTime);

            auto framePtr(m_queue.take());
            if (framePtr.isNull()) {
                continue;
            }
            if (!timer.isValid()) {
                timer.start();
            }
            auto pts = framePtr->pts();
            if (m_seekTime > pts) {
                continue;
            }
            if (d_ptr->firstSeekFrame) {
                d_ptr->seekTime = pts;
                d_ptr->firstSeekFrame = false;
                timer.restart();
            }

            auto audioBuf = audioConverter.convert(framePtr.data());
            auto diff = pts - d_ptr->seekTime
                        - (timer.elapsed() - d_ptr->pauseTime) * d_ptr->speed * 1000;
            // qDebug() << "PTS:"
            //          << QTime::fromMSecsSinceStartOfDay(pts / 1000).toString("hh:mm:ss.zzz")
            //          << "SeekTime:"
            //          << QTime::fromMSecsSinceStartOfDay(d_ptr->seekTime / 1000)
            //                 .toString("hh:mm:ss.zzz")
            //          << "Diff:" << diff;
            {
                auto cleanup = qScopeGuard([&] {
                    setMediaClock(pts);
                    emit positionChanged(pts);
                });
                if (qAbs(diff) > UnWait_Microseconds && !d_ptr->pause) {
                    QMutexLocker locker(&d_ptr->mutex);
                    d_ptr->waitCondition.wait(&d_ptr->mutex, qAbs(diff) / 1000);
                } else if (d_ptr->speed > 1.0) {
                    dropNum++;
                    continue; // speed > 1.0 drop
                }
            }
            writeToDevice(audioBuf);
        }
    } else { // 播放网络媒体
        QElapsedTimer durationTimer;
        qint64 lastPts = 0;
        while (m_runing) {
            checkDefaultAudioOutput(audioDevice);
            checkPause(d_ptr->pauseTime);
            checkSpeed(timer, lastPts);

            auto framePtr(m_queue.take());
            if (framePtr.isNull()) {
                continue;
            }
            auto pts = framePtr->pts();
            if (m_seekTime > pts) {
                continue;
            }
            if (d_ptr->firstSeekFrame) {
                d_ptr->seekTime = pts;
                d_ptr->firstSeekFrame = false;
                lastPts = 0;
            }

            auto audioBuf = audioConverter.convert(framePtr.data());
            double diff = 0;
            if (lastPts > 0) {
                diff = pts - lastPts
                       - (durationTimer.elapsed() - d_ptr->pauseTime) * d_ptr->speed * 1000;
            }
            lastPts = pts;
            {
                auto cleanup = qScopeGuard([&] {
                    durationTimer.restart();
                    setMediaClock(pts);
                    emit positionChanged(pts);
                });
                if (diff > 0 && !d_ptr->pause) {
                    QMutexLocker locker(&d_ptr->mutex);
                    d_ptr->waitCondition.wait(&d_ptr->mutex, diff / 1000);
                } else if (d_ptr->speed > 1.0) {
                    dropNum++;
                    continue; // speed > 1.0 drop
                }
            }
            writeToDevice(audioBuf);
        }
    }
    d_ptr->audioSinkPtr.reset();
    qInfo() << "Audio Drop Num:" << dropNum;
}

void DecoderAudioFrame::buildConnect()
{
    d_ptr->mediaDevicesPtr.reset(new QMediaDevices);
    connect(d_ptr->mediaDevicesPtr.data(),
            &QMediaDevices::audioOutputsChanged,
            this,
            &DecoderAudioFrame::onAudioOutputsChanged);
}

void DecoderAudioFrame::checkDefaultAudioOutput(QAudioDevice &audioDevice)
{
    if (!d_ptr->audioOutputsChanged) {
        return;
    }
    d_ptr->audioOutputsChanged = false;
    if (audioDevice == QMediaDevices::defaultAudioOutput()) {
        return;
    }
    resetAudioOutput();
    audioDevice = QMediaDevices::defaultAudioOutput();
}

void DecoderAudioFrame::checkPause(qint64 &pauseTime)
{
    if (!d_ptr->pause) {
        return;
    }
    QElapsedTimer timerPause;
    timerPause.start();
    while (d_ptr->pause) {
        msleep(Sleep_Queue_Full_Milliseconds);
        QMutexLocker locker(&d_ptr->mutex);
        d_ptr->waitCondition.wait(&d_ptr->mutex);
    }
    if (timerPause.elapsed() > 100) {
        pauseTime += timerPause.elapsed();
    }
}

void DecoderAudioFrame::checkSpeed(QElapsedTimer &timer, qint64 &pauseTime)
{
    auto speed = mediaSpeed();
    if (d_ptr->speed == speed) {
        return;
    }
    d_ptr->speed = speed;
    d_ptr->seekTime = mediaClock() * 1000;
    pauseTime = 0;
    timer.restart();
}

void DecoderAudioFrame::writeToDevice(QByteArray &audioBuf)
{
    if (!d_ptr->audioDevice) {
        return;
    }
    while (audioBuf.size() > 0 && !d_ptr->pause) {
        int byteFree = d_ptr->audioSinkPtr->bytesFree();
        if (byteFree > 0 && byteFree < audioBuf.size()) {
            d_ptr->audioDevice->write(audioBuf.data(), byteFree); // Memory leak
            audioBuf = audioBuf.sliced(byteFree);
        } else {
            d_ptr->audioDevice->write(audioBuf);
            break;
        }
    }
    qApp->processEvents();
}

void printAudioOuputDevice()
{
    QList<QAudioDevice> audioDevices = QMediaDevices::audioOutputs();
    for (const QAudioDevice &audioDevice : qAsConst(audioDevices)) {
        qDebug() << audioDevice.id() << audioDevice.description() << audioDevice.isDefault();
    }
}

QAudioFormat DecoderAudioFrame::resetAudioOutput()
{
    printAudioOuputDevice();

    int sampleSzie = 0;
    auto format = getAudioFormatFromCodecCtx(m_contextInfo->codecCtx(), sampleSzie);
    d_ptr->audioSinkPtr.reset(new QAudioSink(format));
    d_ptr->audioSinkPtr->setBufferSize(format.sampleRate() * sampleSzie / 8);
    d_ptr->audioSinkPtr->setVolume(d_ptr->volume);
    d_ptr->audioDevice = d_ptr->audioSinkPtr->start();
    if (!d_ptr->audioDevice) {
        qWarning() << tr("Create AudioDevice Failed!");
    }
    connect(d_ptr->audioSinkPtr.data(),
            &QAudioSink::stateChanged,
            this,
            &DecoderAudioFrame::onStateChanged);

    return format;
}

} // namespace Ffmpeg
