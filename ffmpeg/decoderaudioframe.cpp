#include "decoderaudioframe.h"
#include "avaudio.h"
#include "codeccontext.h"

#include <QAudioDevice>
#include <QAudioSink>
#include <QMediaDevices>
#include <QMediaFormat>

namespace Ffmpeg {

class DecoderAudioFramePrivate
{
public:
    DecoderAudioFramePrivate(QObject *parent)
        : owner(parent)
    {}
    ~DecoderAudioFramePrivate() {}

    QObject *owner;

    QScopedPointer<QAudioSink> audioSink;
    QIODevice *audioDevice;
    qreal volume = 0.5;

    volatile bool pause = false;
    volatile bool speedChanged = false;
    QMutex mutex;
    QWaitCondition waitCondition;

    qint64 seekTime = 0; // milliseconds
};

DecoderAudioFrame::DecoderAudioFrame(QObject *parent)
    : Decoder<PlayFrame *>(parent)
    , d_ptr(new DecoderAudioFramePrivate(this))
{}

DecoderAudioFrame::~DecoderAudioFrame() {}

void DecoderAudioFrame::stopDecoder()
{
    pause(false);
    Decoder<PlayFrame *>::stopDecoder();
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
    if (d_ptr->audioSink)
        d_ptr->audioSink->setVolume(volume);
}

void DecoderAudioFrame::setSpeed(double speed)
{
    Decoder<PlayFrame *>::setSpeed(speed);
    d_ptr->speedChanged = true;
}

QMutex DecoderAudioFrame::m_mutex;
double DecoderAudioFrame::m_audioClock = 0;

double DecoderAudioFrame::audioClock()
{
    QMutexLocker locker(&m_mutex);
    return m_audioClock;
}

void DecoderAudioFrame::setAudioClock(double time)
{
    QMutexLocker locker(&m_mutex);
    m_audioClock = time;
}

AVSampleFormat converSampleFormat(QAudioFormat::SampleFormat format)
{
    AVSampleFormat type;
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
    QAudioFormat format = resetAudioOutput();
    AVSampleFormat fmt = converSampleFormat(format.sampleFormat());
    setAudioClock(0);
    AVAudio avAudio(m_contextInfo->codecCtx(), fmt);
    qint64 pauseTime = 0;
    QElapsedTimer timer;
    timer.start();
    while (m_runing) {
        checkPause(pauseTime);
        checkSeek(timer, pauseTime);
        checkSpeed(timer, pauseTime);

        if (m_queue.isEmpty()) {
            msleep(1);
            continue;
        }
        QScopedPointer<PlayFrame> framePtr(m_queue.dequeue());

        double duration = 0;
        double pts = 0;
        calculateTime(framePtr->avFrame(), duration, pts);
        if (m_seekTime > pts)
            continue;
        setAudioClock(pts);

        QByteArray audioBuf = avAudio.convert(framePtr.data());
        double speed_ = speed();
        double diff = pts * 1000 - d_ptr->seekTime - (timer.elapsed() - pauseTime) * speed_;
        if (diff > 0) {
            msleep(diff);
        } else if (speed_ > 1.0) {
            continue; // speed > 1.0 drop
        }

        emit positionChanged(pts * 1000);
        writeToDevice(audioBuf);
    }
}

void DecoderAudioFrame::checkPause(qint64 &pauseTime)
{
    QElapsedTimer timerPause;
    timerPause.start();
    while (d_ptr->pause) {
        msleep(100);
        QMutexLocker locker(&d_ptr->mutex);
        d_ptr->waitCondition.wait(&d_ptr->mutex);
    }
    if (timerPause.elapsed() > 100) {
        pauseTime += timerPause.elapsed();
    }
}

void DecoderAudioFrame::checkSeek(QElapsedTimer &timer, qint64 &pauseTime)
{
    if (!m_seek)
        return;

    seekCodec(m_seekTime);
    pauseTime = 0;
    d_ptr->seekTime = m_seekTime * 1000;
    setAudioClock(m_seekTime);
    seekFinish();
    timer.restart();
}

void DecoderAudioFrame::checkSpeed(QElapsedTimer &timer, qint64 &pauseTime)
{
    if (!d_ptr->speedChanged)
        return;

    d_ptr->seekTime = audioClock() * 1000;
    pauseTime = 0;
    d_ptr->speedChanged = false;
    timer.restart();
}

void DecoderAudioFrame::writeToDevice(QByteArray &audioBuf)
{
    while (d_ptr->audioSink->bytesFree() < audioBuf.size()) {
        int byteFree = d_ptr->audioSink->bytesFree();
        if (byteFree > 0) {
            d_ptr->audioDevice->write(audioBuf.data(), byteFree);
            audioBuf = audioBuf.mid(byteFree);
        }
        msleep(1);
    }
    d_ptr->audioDevice->write(audioBuf);
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

    QAudioDevice audioDevice(QMediaDevices::defaultAudioOutput());
    qInfo() << audioDevice.supportedSampleFormats();
    if (!audioDevice.isFormatSupported(format)) {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
    }

    d_ptr->audioSink.reset(new QAudioSink(format));
    d_ptr->audioSink->setBufferSize(format.sampleRate() * sampleSzie / 8);
    d_ptr->audioSink->setVolume(d_ptr->volume);
    d_ptr->audioDevice = d_ptr->audioSink->start();
    return format;
}

} // namespace Ffmpeg
