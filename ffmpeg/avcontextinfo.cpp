#include "avcontextinfo.h"
#include "codeccontext.h"

#include <QDebug>
#include <QTime>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

struct AVContextInfoPrivate{
    QScopedPointer<CodecContext> codecCtx; //解码器上下文
    AVStream *stream;   //流
    int streamIndex = -1; // 索引

    QString error;
};

AVContextInfo::AVContextInfo(QObject *parent)
    : QObject(parent)
    , d_ptr(new AVContextInfoPrivate)
{

}

AVContextInfo::~AVContextInfo()
{

}

QString AVContextInfo::error() const
{
    return d_ptr->error;
}

CodecContext *AVContextInfo::codecCtx()
{
    return d_ptr->codecCtx.data();
}

void AVContextInfo::setIndex(int index)
{
    d_ptr->streamIndex = index;
}

int AVContextInfo::index()
{
    return d_ptr->streamIndex;
}

void AVContextInfo::setStream(AVStream *stream)
{
    d_ptr->stream = stream;

    double frameRate = av_q2d(stream->avg_frame_rate);
    qDebug() << frameRate;
}

AVStream *AVContextInfo::stream()
{
    return d_ptr->stream;
}

bool AVContextInfo::findDecoder()
{
    Q_ASSERT(d_ptr->stream != nullptr);

    AVCodec *codec = avcodec_find_decoder(d_ptr->stream->codecpar->codec_id);
    if (!codec){
        d_ptr->error =  tr("Audio Codec not found.");
        return false;
    }

    d_ptr->codecCtx.reset(new CodecContext(codec));
    if(!d_ptr->codecCtx->isOk()){
        d_ptr->error = tr("Could not open audio codec.");
        return false;
    }

    if(!d_ptr->codecCtx->setParameters(d_ptr->stream->codecpar))
        return false;
    d_ptr->codecCtx->setTimebase(d_ptr->stream->time_base);

    //用于初始化pCodecCtx结构
    if(!d_ptr->codecCtx->open(codec)){
        d_ptr->error = tr("Could not open audio codec.");
        return false;
    }

    qInfo() << tr("Decoder name: ") << codec->name;

    return true;
}

bool AVContextInfo::sendPacket(Packet *packet)
{
    return d_ptr->codecCtx->sendPacket(packet);
}

bool AVContextInfo::receiveFrame(PlayFrame *frame)
{
    return d_ptr->codecCtx->receiveFrame(frame);
}

unsigned char *AVContextInfo::imageBuffer(PlayFrame &frame)
{
    return d_ptr->codecCtx->imageBuffer(frame);
}

void AVContextInfo::clearImageBuffer()
{
    d_ptr->codecCtx->clearImageBuffer();
}

void AVContextInfo::flush()
{
    d_ptr->codecCtx->flush();
}

}
