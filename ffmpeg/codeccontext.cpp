#include "codeccontext.h"
#include "averror.h"
#include "packet.h"
#include "playframe.h"
#include "subtitle.h"

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

namespace Ffmpeg {

struct CodecContextPrivate
{
    ~CodecContextPrivate() { avcodec_free_context(&codecCtx); }
    AVCodecContext *codecCtx; //解码器上下文
    AVError error;
};

CodecContext::CodecContext(const AVCodec *codec, QObject *parent)
    : QObject(parent)
    , d_ptr(new CodecContextPrivate)
{
    d_ptr->codecCtx = avcodec_alloc_context3(codec);
    Q_ASSERT(d_ptr->codecCtx != nullptr);
}

CodecContext::~CodecContext() {}

AVCodecContext *CodecContext::avCodecCtx()
{
    Q_ASSERT(d_ptr->codecCtx != nullptr);
    return d_ptr->codecCtx;
}

bool CodecContext::setParameters(const AVCodecParameters *par)
{
    Q_ASSERT(d_ptr->codecCtx != nullptr);
    int ret = avcodec_parameters_to_context(d_ptr->codecCtx, par);
    if (ret < 0) {
        setError(ret);
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
    Q_ASSERT(d_ptr->codecCtx != nullptr);
    d_ptr->codecCtx->pkt_timebase = timebase;
}

bool CodecContext::open(AVCodec *codec)
{
    int ret = avcodec_open2(d_ptr->codecCtx, codec, NULL);
    if (ret < 0) {
        setError(ret);
        return false;
    }
    return true;
}

bool CodecContext::sendPacket(Packet *packet)
{
    int ret = avcodec_send_packet(d_ptr->codecCtx, packet->avPacket());
    if (ret < 0) {
        setError(ret);
        return false;
    }
    return true;
}

bool CodecContext::receiveFrame(PlayFrame *frame)
{
    int ret = avcodec_receive_frame(d_ptr->codecCtx, frame->avFrame());
    if (ret >= 0) {
        return true;
    }
    if (ret != -11) { // Resource temporarily unavailable
        setError(ret);
    }
    return false;
}

bool CodecContext::decodeSubtitle2(Subtitle *subtitle, Packet *packet)
{
    int got_sub_ptr = 0;
    int ret = avcodec_decode_subtitle2(d_ptr->codecCtx,
                                       subtitle->avSubtitle(),
                                       &got_sub_ptr,
                                       packet->avPacket());
    if (ret < 0 || got_sub_ptr <= 0) {
        setError(ret);
        return false;
    }
    return true;
}

bool CodecContext::imageAlloc(PlayFrame &frame)
{
    int ret = av_image_alloc(frame.avFrame()->data,
                             frame.avFrame()->linesize,
                             width(),
                             height(),
                             AV_PIX_FMT_RGB32,
                             1);

    if (ret < 0) {
        setError(ret);
        return false;
    }
    return true;
}

int CodecContext::width()
{
    return d_ptr->codecCtx->width;
}

int CodecContext::height()
{
    return d_ptr->codecCtx->height;
}

void CodecContext::flush()
{
    avcodec_flush_buffers(d_ptr->codecCtx);
}

AVError CodecContext::avError()
{
    return d_ptr->error;
}

void CodecContext::setError(int errorCode)
{
    d_ptr->error.setError(errorCode);
    emit error(d_ptr->error);
}

} // namespace Ffmpeg
