#include "decoder.h"
#include "codeccontext.h"

#include <QDebug>

extern "C"{
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

Decoder::Decoder(QObject *parent)
    : QThread(parent)
{

}

Decoder::~Decoder()
{
    stopDecoder();
}

void Decoder::startDecoder(AVContextInfo *contextInfo)
{
    stopDecoder();
    m_contextInfo = contextInfo;
    m_runing = true;
    start();
}

void Decoder::stopDecoder()
{
    m_runing = false;
    if(isRunning()){
        quit();
        wait();
    }
}

void Decoder::run()
{
    Q_ASSERT(m_contextInfo != nullptr);
    runDecoder();
}

}
