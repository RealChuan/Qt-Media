#include "decoderaudioframe.h"
#include "avaudio.h"

#include <QAudioOutput>

extern "C"{
#include <libavutil/time.h>
#include <libavformat/avformat.h>
}

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
    QAtomicInteger<qint64> seek = 0; // ms
};

DecoderAudioFrame::DecoderAudioFrame(QObject *parent)
    : Decoder(parent)
    , d_ptr(new DecoderAudioFramePrivate(this))
{

}

DecoderAudioFrame::~DecoderAudioFrame()
{
    stopDecoder();
}

void DecoderAudioFrame::setSeek(qint64 seek)
{
    d_ptr->seek = seek;
}

double DecoderAudioFrame::audioClock()
{
    QMutexLocker locker(&g_Mutex);
    return g_Audio_Clock;
}

void DecoderAudioFrame::runDecoder()
{
    AVAudio avAudio(m_contextInfo->codecCtx());

    QElapsedTimer timer;
    timer.start();
    while(m_runing){
        if(m_queue.isEmpty()){
            msleep(1);
            continue;
        }

        PlayFrame frame = m_queue.takeFirst();

        double duration = 0;
        double pts = 0;
        calculateTime(frame.avFrame(), duration, pts);

        {
            QMutexLocker locker(&g_Mutex);
            g_Audio_Clock = pts;
        }

        QByteArray audioBuf = avAudio.convert(&frame, m_contextInfo->codecCtx());

        double diff = pts * 1000 - timer.elapsed() - d_ptr->seek;
        if(diff > 0.0)
            msleep(diff);

        emit positionChanged(pts * 1000);

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

}
