#include "decoderaudioframe.h"
#include "avaudio.h"
#include "codeccontext.h"

#include <QAudioDevice>
#include <QAudioSink>
#include <QMediaDevices>
#include <QMediaFormat>

namespace Ffmpeg {

struct DecoderAudioFrame::DecoderAudioFramePrivate
{
    QScopedPointer<QAudioSink> audioSinkPtr;
    QIODevice *audioDevice;
    qreal volume = 0.5;

    volatile bool pause = false;
    volatile bool speedChanged = false;
    QMutex mutex;
    QWaitCondition waitCondition;

    qint64 seekTime = 0; // milliseconds
    volatile bool firstSeekFrame = false;

    QScopedPointer<QMediaDevices> mediaDevicesPtr;
    volatile bool audioOutputsChanged = false;
    volatile bool isLocalFile = true;
};

DecoderAudioFrame::DecoderAudioFrame(QObject *parent)
    : Decoder<Frame *>(parent)
    , d_ptr(new DecoderAudioFramePrivate)
{
    buildConnect();
}

DecoderAudioFrame::~DecoderAudioFrame() {}

void DecoderAudioFrame::stopDecoder()
{
    pause(false);
    Decoder<Frame *>::stopDecoder();
}

void DecoderAudioFrame::pause(bool state)
{
    if (!isRunning())
        return;

    d_ptr->pause = state;
    if (state)
        return;
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

void DecoderAudioFrame::setSpeed(double speed)
{
    Decoder<Frame *>::setSpeed(speed);
    d_ptr->speedChanged = true;
}

void DecoderAudioFrame::setIsLocalFile(bool isLocalFile)
{
    d_ptr->isLocalFile = isLocalFile;
}

void DecoderAudioFrame::onStateChanged(QAudio::State state)
{
    if (d_ptr->audioSinkPtr.isNull()) {
        return;
    }
    if (d_ptr->audioSinkPtr->error() != QAudio::NoError) {
        qWarning() << tr("QAudioSink Error:") << d_ptr->audioSinkPtr->error();
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

AVSampleFormat converSampleFormat(QAudioFormat::SampleFormat format)
{
    AVSampleFormat type = AV_SAMPLE_FMT_NONE;
    switch (format) {
    case QAudioFormat::Unknown: type = AV_SAMPLE_FMT_NONE; break;
    case QAudioFormat::UInt8: type = AV_SAMPLE_FMT_U8; break;
    case QAudioFormat::Int16: type = AV_SAMPLE_FMT_S16; break;
    case QAudioFormat::Int32: type = AV_SAMPLE_FMT_S32; break;
    case QAudioFormat::Float: type = AV_SAMPLE_FMT_FLT; break;
    default: break;
    }
    return type;
}

void DecoderAudioFrame::runDecoder()
{
    QAudioDevice audioDevice(QMediaDevices::defaultAudioOutput());
    QAudioFormat format = resetAudioOutput();
    AVSampleFormat fmt = converSampleFormat(format.sampleFormat());
    d_ptr->seekTime = 0;
    setClock(0);
    AVAudio avAudio(m_contextInfo->codecCtx(), fmt);
    QElapsedTimer timer;
    if (d_ptr->isLocalFile) { // 音频播放依赖外部时钟，适用于本地文件播放
        qint64 pauseTime = 0;
        timer.start();
        while (m_runing) {
            checkDefaultAudioOutput(audioDevice);
            checkPause(pauseTime);
            checkSeek(timer, pauseTime);
            checkSpeed(timer, pauseTime);

            QScopedPointer<Frame> framePtr(m_queue.dequeue());
            if (framePtr.isNull()) {
                msleep(Sleep_Queue_Empty_Milliseconds);
                continue;
            }
            double pts = framePtr->pts();
            if (m_seekTime > pts) {
                continue;
            }
            if (d_ptr->firstSeekFrame) {
                d_ptr->seekTime = pts * 1000;
                d_ptr->firstSeekFrame = false;
                timer.restart();
            }

            QByteArray audioBuf = avAudio.convert(framePtr.data());
            double speed_ = speed();
            double diff = pts * 1000 - d_ptr->seekTime - (timer.elapsed() - pauseTime) * speed_;
            {
                auto cleanup = qScopeGuard([&] {
                    setClock(pts);
                    emit positionChanged(pts * 1000);
                });
                if (diff > UnWait_Milliseconds && !m_seek && !d_ptr->pause) {
                    QMutexLocker locker(&d_ptr->mutex);
                    d_ptr->waitCondition.wait(&d_ptr->mutex, diff);
                    //msleep(diff);
                } else if (speed_ > 1.0) {
                    continue; // speed > 1.0 drop
                }
            }
            writeToDevice(audioBuf);
        }
    } else { // 播放网络媒体
        QElapsedTimer timer_;
        qint64 lastPts = 0;
        while (m_runing) {
            checkDefaultAudioOutput(audioDevice);
            qint64 pauseTime = 0;
            checkPause(pauseTime);
            checkSeek(timer, lastPts);
            checkSpeed(timer, lastPts);
            if (m_queue.isEmpty()) {
                msleep(Sleep_Queue_Empty_Milliseconds);
                continue;
            }

            QScopedPointer<Frame> framePtr(m_queue.dequeue());
            double pts = framePtr->pts();
            if (m_seekTime > pts) {
                continue;
            }

            QByteArray audioBuf = avAudio.convert(framePtr.data());
            double speed_ = speed();
            double diff = 0;
            auto pts_ = pts * 1000;
            if (lastPts > 0) {
                diff = pts_ - lastPts - (timer_.elapsed() - pauseTime) * speed_;
            }
            lastPts = pts_;
            {
                auto cleanup = qScopeGuard([&] {
                    timer_.restart();
                    setClock(pts);
                    emit positionChanged(pts * 1000);
                });
                if (diff > 0 && !m_seek && !d_ptr->pause) {
                    QMutexLocker locker(&d_ptr->mutex);
                    d_ptr->waitCondition.wait(&d_ptr->mutex, diff);
                    //msleep(diff);
                } else if (speed_ > 1.0) {
                    continue; // speed > 1.0 drop
                }
            }
            writeToDevice(audioBuf);
        }
    }
    d_ptr->audioSinkPtr.reset();
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

void DecoderAudioFrame::checkSeek(QElapsedTimer &timer, qint64 &pauseTime)
{
    if (!m_seek) {
        return;
    }
    clear();
    pauseTime = 0;
    setClock(m_seekTime);
    d_ptr->seekTime = m_seekTime * 1000;
    auto latchPtr = m_latchPtr.lock();
    if (latchPtr) {
        latchPtr->countDown();
    }
    seekFinish();
    d_ptr->firstSeekFrame = true;
}

void DecoderAudioFrame::checkSpeed(QElapsedTimer &timer, qint64 &pauseTime)
{
    if (!d_ptr->speedChanged) {
        return;
    }
    d_ptr->seekTime = clock() * 1000;
    pauseTime = 0;
    d_ptr->speedChanged = false;
    timer.restart();
}

void DecoderAudioFrame::writeToDevice(QByteArray &audioBuf)
{
    while (audioBuf.size() > 0 && !m_seek && !d_ptr->pause) {
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
    AVCodecContext *ctx = m_contextInfo->codecCtx()->avCodecCtx();
    int sampleSzie = 0;
    QAudioFormat format;
    //format.setCodec("audio/pcm");
    format.setSampleRate(ctx->sample_rate);
    format.setChannelCount(ctx->channels);
    //format.setByteOrder(QAudioFormat::LittleEndian);
    if (ctx->sample_fmt == AV_SAMPLE_FMT_U8) {
        format.setSampleFormat(QAudioFormat::UInt8);
        sampleSzie = 8 * av_get_bytes_per_sample(AV_SAMPLE_FMT_U8);
    } else if (ctx->sample_fmt == AV_SAMPLE_FMT_S16) {
        format.setSampleFormat(QAudioFormat::Int16);
        sampleSzie = 8 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    } /*else if (ctx->sample_fmt == AV_SAMPLE_FMT_S32) {
        format.setSampleFormat(QAudioFormat::Int32);
        sampleSzie = 8 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S32);
    } */
    else if (ctx->sample_fmt == AV_SAMPLE_FMT_FLT) {
        format.setSampleFormat(QAudioFormat::Float);
        sampleSzie = 8 * av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT);
    } else {
        format.setSampleFormat(QAudioFormat::Int16);
        sampleSzie = 8 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    }
    printAudioOuputDevice();
    QAudioDevice audioDevice(QMediaDevices::defaultAudioOutput());
    qInfo() << audioDevice.supportedSampleFormats();
    if (!audioDevice.isFormatSupported(format)) {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
    }

    d_ptr->audioSinkPtr.reset(new QAudioSink(format));
    d_ptr->audioSinkPtr->setBufferSize(format.sampleRate() * sampleSzie / 8);
    d_ptr->audioSinkPtr->setVolume(d_ptr->volume);
    d_ptr->audioDevice = d_ptr->audioSinkPtr->start();
    connect(d_ptr->audioSinkPtr.data(),
            &QAudioSink::stateChanged,
            this,
            &DecoderAudioFrame::onStateChanged);

    return format;
}

} // namespace Ffmpeg
