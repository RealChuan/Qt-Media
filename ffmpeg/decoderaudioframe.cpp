#include "decoderaudioframe.h"
#include "avaudio.h"

#include <QAudioOutput>

namespace Ffmpeg {

static QMutex g_Mutex;
static double g_Audio_Clock = 0.0;

class DecoderAudioFramePrivate{
public:
    DecoderAudioFramePrivate(QObject *parent)
        : owner(parent){
        QAudioFormat format;
        format.setSampleRate(SAMPLE_RATE);
        format.setChannelCount(CHANNELS);
        format.setSampleSize(16);
        format.setCodec("audio/pcm");
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::SignedInt);
        audioOutput = new QAudioOutput(format, owner);
        audioOutput->setVolume(0.5);
        audioDevice = audioOutput->start();

        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
        qInfo() << info.supportedCodecs();
        if (!info.isFormatSupported(format)) {
            qWarning() << "Raw audio format not supported by backend, cannot play audio.";
        }
    }

    QObject *owner;

    QAudioOutput *audioOutput;
    QIODevice *audioDevice;

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
    d_ptr->audioOutput->setVolume(volume);
}

void DecoderAudioFrame::setSpeed(double speed)
{
    Decoder<PlayFrame>::setSpeed(speed);
    d_ptr->speedChanged = true;
}

double DecoderAudioFrame::audioClock()
{
    QMutexLocker locker(&g_Mutex);
    return g_Audio_Clock;
}

void setAudioClock(double time)
{
    QMutexLocker locker(&g_Mutex);
    g_Audio_Clock = time;
}

void DecoderAudioFrame::runDecoder()
{
    setAudioClock(0);
    AVAudio avAudio(m_contextInfo->codecCtx());
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
        PlayFrame frame = m_queue.takeFirst();

        double duration = 0;
        double pts = 0;
        calculateTime(frame.avFrame(), duration, pts);
        if(m_seekTime > pts)
            continue;
        setAudioClock(pts);

        QByteArray audioBuf = avAudio.convert(&frame, m_contextInfo->codecCtx());
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

}
