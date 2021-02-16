#include "codeccontext.h"
#include "packet.h"
#include "playframe.h"

#include <QDebug>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

namespace Ffmpeg {

CodecContext::CodecContext(const AVCodec *codec, QObject *parent)
    : QObject(parent)
{
    m_codecCtx = avcodec_alloc_context3(codec);
    Q_ASSERT(m_codecCtx != nullptr);
}

CodecContext::~CodecContext()
{
    Q_ASSERT(m_codecCtx != nullptr);
    avcodec_free_context(&m_codecCtx);
}

AVCodecContext *CodecContext::avCodecCtx()
{
    Q_ASSERT(m_codecCtx != nullptr);
    return m_codecCtx;
}

bool CodecContext::setParameters(const AVCodecParameters *par)
{
    Q_ASSERT(m_codecCtx != nullptr);
    if(avcodec_parameters_to_context(m_codecCtx, par) < 0){
        qWarning() << "avcodec_parameters_to_context";
        return false;
    }
    //qDebug() << m_codecCtx->framerate.num;
    //qInfo() << tr("Width: ") << m_codecCtx->width << tr(" Height: ") << m_codecCtx->height;
    //qInfo() << tr("Channels: ") << m_codecCtx->channels;
    //qInfo() << tr("sample_fmt: ") << m_codecCtx->sample_fmt;
    //qInfo() << tr("sample_rate: ") << m_codecCtx->sample_rate;
    //qInfo() << tr("channel_layout: ") << m_codecCtx->channel_layout;
    return true;
}

void CodecContext::setTimebase(const AVRational &timebase)
{
    Q_ASSERT(m_codecCtx != nullptr);
    m_codecCtx->pkt_timebase = timebase;
}

bool CodecContext::open(AVCodec *codec)
{
    if (avcodec_open2(m_codecCtx, codec, NULL) < 0){
        return false;
    }
    return true;
}

bool CodecContext::sendPacket(Packet *packet)
{
    if(avcodec_send_packet(m_codecCtx, packet->avPacket()) < 0){
        return false;
    }
    return true;
}

bool CodecContext::receiveFrame(PlayFrame *frame)
{
    int ret = avcodec_receive_frame(m_codecCtx, frame->avFrame());

    QString error;
    switch (ret) {
    case 0: return true;
    case AVERROR_EOF:
        error = tr("avcodec_receive_frame(): the decoder has been fully flushed");
        break;
    case AVERROR(EAGAIN):
        error = tr("avcodec_receive_frame(): output is not available in this state - user must try to send new input");
        break;
    case AVERROR(EINVAL):
        error = tr("avcodec_receive_frame(): codec not opened, or it is an encoder");
        break;
    default:
        error = tr("avcodec_receive_frame(): legitimate decoding errors");
        break;
    }
    //qWarning() << error;
    return false;
}

unsigned char *CodecContext::imageBuffer(PlayFrame &frame)
{
    m_out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB32, width(), height(), 1));
    av_image_fill_arrays(frame.avFrame()->data, frame.avFrame()->linesize, m_out_buffer,
                         AV_PIX_FMT_RGB32, width(), height(), 1);

    return m_out_buffer;
}

void CodecContext::clearImageBuffer()
{
    if(m_out_buffer != nullptr)
        av_free(m_out_buffer);
}

int CodecContext::width()
{
    return m_codecCtx->width;
}

int CodecContext::height()
{
    return m_codecCtx->height;
}

void CodecContext::flush()
{
    avcodec_flush_buffers(m_codecCtx);
}

}
