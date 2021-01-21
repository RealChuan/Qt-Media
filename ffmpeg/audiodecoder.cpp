#include "audiodecoder.h"
#include "avaudio.h"
#include "avcontextinfo.h"
#include "packet.h"
#include "playframe.h"

#include <utils/taskqueue.h>
#include <utils/utils.h>

#include <QAudioOutput>

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

    Utils::Queue<Packet> packetQueue;
    AVContextInfo *audioInfo;

    QAudioOutput *audioOutput;
    QIODevice *audioDevice;

    bool runing = true;
};

AudioDecoder::AudioDecoder(QObject *parent)
    : QThread(parent)
    , d_ptr(new AudioDecoderPrivate(this))
{

}

AudioDecoder::~AudioDecoder()
{
    stopDecoder();
}

void AudioDecoder::startDecoder(AVContextInfo *audioInfo)
{
    stopDecoder();
    d_ptr->audioInfo = audioInfo;
    d_ptr->runing = true;
    start();
}

void AudioDecoder::stopDecoder()
{
    d_ptr->runing = false;
    if(isRunning()){
        quit();
        wait();
    }
}

void AudioDecoder::append(const Packet &packet)
{
    d_ptr->packetQueue.append(packet);
}

void AudioDecoder::run()
{
    Q_ASSERT(d_ptr->audioInfo != nullptr);

    PlayFrame frame;
    AVAudio avAudio(d_ptr->audioInfo->codecCtx());

    while(d_ptr->runing){
        msleep(10);

        if(d_ptr->packetQueue.isEmpty())
            continue;

        Packet packet = d_ptr->packetQueue.takeFirst();

        if(!d_ptr->audioInfo->sendPacket(&packet)){
            continue;
        }
        if(!d_ptr->audioInfo->receiveFrame(&frame)){
            continue;
        }

        QByteArray audioBuf = avAudio.convert(&frame, d_ptr->audioInfo->codecCtx());

        while(d_ptr->audioOutput->bytesFree() < audioBuf.size()){
            d_ptr->audioDevice->write(audioBuf.data(), d_ptr->audioOutput->bytesFree());
            audioBuf = audioBuf.mid(d_ptr->audioOutput->bytesFree());
            msleep(10);
        }
        d_ptr->audioDevice->write(audioBuf);
    }
}

}
