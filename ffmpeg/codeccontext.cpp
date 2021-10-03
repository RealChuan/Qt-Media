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
    int ret = avcodec_parameters_to_context(m_codecCtx, par);
    if (ret < 0) {
        emit error(AVError(ret));
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
    int ret = avcodec_open2(m_codecCtx, codec, NULL);
    if (ret < 0) {
        emit error(AVError(ret));
        return false;
    }
    return true;
}

bool CodecContext::sendPacket(Packet *packet)
{
    int ret = avcodec_send_packet(m_codecCtx, packet->avPacket());
    if (ret < 0) {
        emit error(AVError(ret));
        return false;
    }
    return true;
}

bool CodecContext::receiveFrame(PlayFrame *frame)
{
    int ret = avcodec_receive_frame(m_codecCtx, frame->avFrame());
    if (ret >= 0)
        return true;
    if (ret != -11) // Resource temporarily unavailable
        emit error(AVError(ret));
    return false;
}

bool CodecContext::decodeSubtitle2(Subtitle *subtitle, Packet *packet)
{
    int got_sub_ptr = 0;
    int ret = avcodec_decode_subtitle2(m_codecCtx,
                                       subtitle->avSubtitle(),
                                       &got_sub_ptr,
                                       packet->avPacket());
    if (ret < 0 || got_sub_ptr <= 0) {
        emit error(AVError(ret));
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
        emit error(AVError(ret));
        return false;
    }
    return true;
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

} // namespace Ffmpeg
