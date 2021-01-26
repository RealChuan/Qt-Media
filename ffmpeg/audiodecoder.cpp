#include "audiodecoder.h"
#include "avcontextinfo.h"
#include "decoderaudioframe.h"

#include <QDebug>

namespace Ffmpeg {

class AudioDecoderPrivate{
public:
    AudioDecoderPrivate(QObject *parent)
        : owner(parent){
        decoderAudioFrame = new DecoderAudioFrame(owner);
    }
    QObject *owner;

    DecoderAudioFrame *decoderAudioFrame;
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

    d_ptr->decoderAudioFrame->startDecoder(m_formatContext, m_contextInfo);

    while(m_runing){
        if(m_queue.isEmpty()){
            msleep(1);
            continue;
        }

        Packet packet = m_queue.takeFirst();

        if(!m_contextInfo->sendPacket(&packet)){
            continue;
        }

        while(m_contextInfo->receiveFrame(&frame)){ // 一个packet 一个或多个音频帧
            d_ptr->decoderAudioFrame->append(frame);
        }

        while(d_ptr->decoderAudioFrame->size() > 10)
            msleep(40);
    }

    while(m_runing && d_ptr->decoderAudioFrame->size() != 0)
        msleep(40);

    d_ptr->decoderAudioFrame->stopDecoder();
}

}
