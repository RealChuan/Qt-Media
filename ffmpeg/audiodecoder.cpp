#include "audiodecoder.h"
#include "avaudio.h"
#include "avcontextinfo.h"
#include "playframe.h"
#include "codeccontext.h"
#include "formatcontext.h"

#include <utils/utils.h>

#include <QAudioOutput>

extern "C"{
#include <libavutil/time.h>
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

static QMutex g_Mutex;
static double g_Audio_Clock = 0.0;

class AudioDecoderPrivate{
public:
    AudioDecoderPrivate(QObject *parent)
        : owner(parent){
        QAudioFormat format;
        format.setSampleRate(SAMPLE_RATE);
        format.setChannelCount(2);
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
};

AudioDecoder::AudioDecoder(QObject *parent)
    : Decoder(parent)
    , d_ptr(new AudioDecoderPrivate(this))
{
    d_ptr->audioOutput->format();
}

AudioDecoder::~AudioDecoder()
{
    stopDecoder();
}

double AudioDecoder::audioClock()
{
    QMutexLocker locker(&g_Mutex);
    return g_Audio_Clock;
}

void AudioDecoder::runDecoder()
{
    PlayFrame frame;
    AVAudio avAudio(m_contextInfo->codecCtx());

    QElapsedTimer timer;
    timer.start();

    while(m_runing){
        if(m_packetQueue.isEmpty()){
            msleep(1);
            continue;
        }

        Packet packet = m_packetQueue.takeFirst();

        if(!m_contextInfo->sendPacket(&packet)){
            continue;
        }

        while(m_contextInfo->receiveFrame(&frame)){ // 一个packet 一个或多个音频帧
            double duration = 0;
            double pts = 0;
            int64_t pos = 0;
            calculateTime(frame, duration, pts, pos);

            {
                QMutexLocker locker(&g_Mutex);
                g_Audio_Clock = pts;
            }

            QByteArray audioBuf = avAudio.convert(&frame, m_contextInfo->codecCtx());

            double diff = pts * 1000 - timer.elapsed();
            if(diff > 0.0)
                msleep(diff);

            while(d_ptr->audioOutput->bytesFree() < audioBuf.size()){
                d_ptr->audioDevice->write(audioBuf.data(), d_ptr->audioOutput->bytesFree());
                audioBuf = audioBuf.mid(d_ptr->audioOutput->bytesFree());
                msleep(1);
            }
            d_ptr->audioDevice->write(audioBuf);
        }
    }
}

void AudioDecoder::calculateTime(PlayFrame &frame, double &duration, double &pts, int64_t &pos)
{
    AVRational tb{1, frame.avFrame()->sample_rate};
    //pts = (frame.avFrame()->pts == AV_NOPTS_VALUE) ? NAN : frame.avFrame()->pts * av_q2d(tb);
    pos = frame.avFrame()->pkt_pos;
    //duration = av_q2d(AVRational{frame.avFrame()->nb_samples, frame.avFrame()->sample_rate});
    //qDebug() << "audio: " << duration << pts << pos << frame.avFrame()->nb_samples << frame.avFrame()->sample_rate;

    tb = m_contextInfo->stream()->time_base;
    AVRational frame_rate = av_guess_frame_rate(m_formatContext->avFormatContext(), m_contextInfo->stream(), NULL);
    // 当前帧播放时长
    duration = (frame_rate.num && frame_rate.den ? av_q2d(AVRational{frame_rate.den, frame_rate.num}) : 0);
    // 当前帧显示时间戳
    pts = (frame.avFrame()->pts == AV_NOPTS_VALUE) ? NAN : frame.avFrame()->pts * av_q2d(tb);
    //qDebug() << "audio: " << duration << pts;
}

}
