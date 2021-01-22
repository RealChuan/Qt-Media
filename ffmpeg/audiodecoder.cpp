#include "audiodecoder.h"
#include "avaudio.h"
#include "avcontextinfo.h"
#include "playframe.h"
#include "codeccontext.h"

#include <utils/utils.h>

#include <QAudioOutput>

extern "C"{
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

class AudioDecoderPrivate{
public:
    AudioDecoderPrivate(QObject *parent)
        : owner(parent){
        QAudioFormat format;
        format.setSampleRate(44100);
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

}

AudioDecoder::~AudioDecoder()
{
    stopDecoder();
}

void AudioDecoder::runDecoder()
{
    PlayFrame frame;
    AVAudio avAudio(m_contextInfo->codecCtx());

    while(m_runing){
        msleep(10);

        if(m_packetQueue.isEmpty())
            continue;

        Packet packet = m_packetQueue.takeFirst();

        if(!m_contextInfo->sendPacket(&packet)){
            continue;
        }
        if(!m_contextInfo->receiveFrame(&frame)){
            continue;
        }

        double duration = 0;
        double pts = 0;
        int64_t pos = 0;
        calculateTime(frame, duration, pts, pos);

        QByteArray audioBuf = avAudio.convert(&frame, m_contextInfo->codecCtx());

        while(d_ptr->audioOutput->bytesFree() < audioBuf.size()){
            d_ptr->audioDevice->write(audioBuf.data(), d_ptr->audioOutput->bytesFree());
            audioBuf = audioBuf.mid(d_ptr->audioOutput->bytesFree());
            //msleep(10);
        }
        d_ptr->audioDevice->write(audioBuf);
    }
}

void AudioDecoder::calculateTime(PlayFrame &frame, double &duration, double &pts, int64_t &pos)
{
    AVRational tb{1, frame.avFrame()->sample_rate};
    pts = (frame.avFrame()->pts == AV_NOPTS_VALUE) ? NAN : frame.avFrame()->pts * av_q2d(tb);
    pos = frame.avFrame()->pkt_pos;
    duration = av_q2d(AVRational{frame.avFrame()->nb_samples, frame.avFrame()->sample_rate});
    qDebug() << "audio: " << duration << pts << pos;
}

}
