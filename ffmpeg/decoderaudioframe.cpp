#include "decoderaudioframe.h"
#include "avaudio.h"
#include "codeccontext.h"

#include <QAudioOutput>

namespace Ffmpeg {

class DecoderAudioFramePrivate{
public:
    DecoderAudioFramePrivate(QObject *parent)
        : owner(parent) {}
    ~DecoderAudioFramePrivate() {}

    QObject *owner;

    QScopedPointer<QAudioOutput> audioOutput;
    QIODevice *audioDevice;
    qreal volume = 0.5;

    volatile bool pause = false;
    volatile bool speedChanged = false;
    QMutex mutex;
    QWaitCondition waitCondition;

    qint64 seekTime = 0; // milliseconds
};

DecoderAudioFrame::DecoderAudioFrame(QObject *parent)
    : Decoder<PlayFrame>(parent)
    , d_ptr(new DecoderAudioFramePrivate(this))
{

}

DecoderAudioFrame::~DecoderAudioFrame()
{

}

void DecoderAudioFrame::stopDecoder()
{
    pause(false);
    Decoder<PlayFrame>::stopDecoder();
}

void DecoderAudioFrame::pause(bool state)
{
    if(!isRunning())
        return;

    d_ptr->pause = state;
    if(state)
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
    if(d_ptr->audioOutput)
        d_ptr->audioOutput->setVolume(volume);
}

void DecoderAudioFrame::setSpeed(double speed)
{
    Decoder<PlayFrame>::setSpeed(speed);
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

AVSampleFormat converSampleFormat(QAudioFormat::SampleType format)
{
    AVSampleFormat type;
    switch (format) {
    case QAudioFormat::Float:
        type = AV_SAMPLE_FMT_FLT;
        break;
    default:
    case QAudioFormat::SignedInt:
        type = AV_SAMPLE_FMT_S32;
        break;
    }
    return type;
}

void DecoderAudioFrame::runDecoder()
{
    QAudioFormat format = resetAudioOutput();
    AVSampleFormat fmt = converSampleFormat(format.sampleType());
    setAudioClock(0);
    AVAudio avAudio(m_contextInfo->codecCtx(), fmt);
    qint64 pauseTime = 0;
    QElapsedTimer timer;
    timer.start();
    while(m_runing){
        checkPause(pauseTime);
        checkSeek(timer, pauseTime);
        checkSpeed(timer, pauseTime);

        if(m_queue.isEmpty()){
            msleep(1);
            continue;
        }
        PlayFrame frame = m_queue.dequeue();

        double duration = 0;
        double pts = 0;
        calculateTime(frame.avFrame(), duration, pts);
        if(m_seekTime > pts)
            continue;
        setAudioClock(pts);

        QByteArray audioBuf = avAudio.convert(&frame);
        double speed_ = speed();
        double diff = pts * 1000 - d_ptr->seekTime - (timer.elapsed() - pauseTime) * speed_;
        if(diff > 0){
            msleep(diff);
        }else if(speed_ > 1.0){
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
    while(d_ptr->pause){
        msleep(100);
        QMutexLocker locker(&d_ptr->mutex);
        d_ptr->waitCondition.wait(&d_ptr->mutex);
    }
    if(timerPause.elapsed() > 100){
        pauseTime += timerPause.elapsed();
    }
}

void DecoderAudioFrame::checkSeek(QElapsedTimer &timer, qint64 &pauseTime)
{
    if(!m_seek)
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
    if(!d_ptr->speedChanged)
        return;

    d_ptr->seekTime = audioClock() * 1000;
    pauseTime = 0;
    d_ptr->speedChanged = false;
    timer.restart();
}

void DecoderAudioFrame::writeToDevice(QByteArray &audioBuf)
{
    while(d_ptr->audioOutput->bytesFree() < audioBuf.size()){
        int byteFree = d_ptr->audioOutput->bytesFree();
        if(byteFree > 0){
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
    QAudioFormat format;
    format.setCodec("audio/pcm");
    format.setSampleRate(ctx->sample_rate);
    format.setChannelCount(ctx->channels);
    format.setByteOrder(QAudioFormat::LittleEndian);
    if (ctx->sample_fmt == AV_SAMPLE_FMT_FLT) {
        format.setSampleType(QAudioFormat::Float);
        format.setSampleSize(8 * av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT));
    } else {
        format.setSampleType(QAudioFormat::SignedInt);
        format.setSampleSize(8 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S32));
    }

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    qInfo() << info.supportedCodecs();
    if (!info.isFormatSupported(format)) {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
    }

    d_ptr->audioOutput.reset(new QAudioOutput(format));
    d_ptr->audioOutput->setBufferSize(format.sampleRate() * format.sampleSize() / 8);
    d_ptr->audioOutput->setVolume(d_ptr->volume);
    d_ptr->audioDevice = d_ptr->audioOutput->start();
    return format;
}

}
