#include "codeccontext.h"
#include "averror.h"
#include "frame.hpp"
#include "packet.h"
#include "subtitle.h"

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace Ffmpeg {

struct CodecContext::CodecContextPrivate
{
    ~CodecContextPrivate() { avcodec_free_context(&codecCtx); }
    AVCodec *codec = nullptr;
    AVCodecContext *codecCtx = nullptr; //解码器上下文
    AVError error;
};

CodecContext::CodecContext(AVCodec *codec, QObject *parent)
    : QObject(parent)
    , d_ptr(new CodecContextPrivate)
{
    d_ptr->codec = codec;
    d_ptr->codecCtx = avcodec_alloc_context3(codec);
    Q_ASSERT(d_ptr->codecCtx != nullptr);
}

CodecContext::~CodecContext() {}

void CodecContext::copyToCodecParameters(CodecContext *dst)
{
    auto dstCodecCtx = dst->d_ptr->codecCtx;

    switch (mediaType()) {
    case AVMEDIA_TYPE_AUDIO:
        dstCodecCtx->sample_rate = d_ptr->codecCtx->sample_rate;
        dstCodecCtx->channel_layout = d_ptr->codecCtx->channel_layout;
        dstCodecCtx->channels = av_get_channel_layout_nb_channels(dstCodecCtx->channel_layout);
        /* take first format from list of supported formats */
        dstCodecCtx->sample_fmt = d_ptr->codec->sample_fmts[0];
        dstCodecCtx->time_base = AVRational{1, dstCodecCtx->sample_rate};
        break;
    case AVMEDIA_TYPE_VIDEO:
        dstCodecCtx->height = d_ptr->codecCtx->height;
        dstCodecCtx->width = d_ptr->codecCtx->width;
        dstCodecCtx->sample_aspect_ratio = d_ptr->codecCtx->sample_aspect_ratio;
        /* take first format from list of supported formats */
        if (d_ptr->codec->pix_fmts) {
            dstCodecCtx->pix_fmt = d_ptr->codec->pix_fmts[0];
        } else {
            dstCodecCtx->pix_fmt = d_ptr->codecCtx->pix_fmt;
        }
        /* video time_base can be set to whatever is handy and supported by encoder */
        dstCodecCtx->time_base = av_inv_q(d_ptr->codecCtx->framerate);
        break;
    default: break;
    }
}

AVCodecContext *CodecContext::avCodecCtx()
{
    return d_ptr->codecCtx;
}

bool CodecContext::setParameters(const AVCodecParameters *par)
{
    int ret = avcodec_parameters_to_context(d_ptr->codecCtx, par);
    if (ret < 0) {
        setError(ret);
        return false;
    }
    return true;
}

void CodecContext::setTimebase(const AVRational &timebase)
{
    Q_ASSERT(d_ptr->codecCtx != nullptr);
    d_ptr->codecCtx->pkt_timebase = timebase;
}

void CodecContext::setThreadCount(int threadCount)
{
    Q_ASSERT(d_ptr->codecCtx != nullptr && d_ptr->codec != nullptr);
    if (d_ptr->codec->capabilities & AV_CODEC_CAP_FRAME_THREADS) {
        d_ptr->codecCtx->thread_type = FF_THREAD_FRAME;
    } else if (d_ptr->codec->capabilities & AV_CODEC_CAP_SLICE_THREADS) {
        d_ptr->codecCtx->thread_type = FF_THREAD_SLICE;
    }
    d_ptr->codecCtx->thread_count = threadCount;
}

void CodecContext::setFrameRate(const AVRational &frameRate)
{
    d_ptr->codecCtx->framerate = frameRate;
}

void CodecContext::setFlags(int flags)
{
    d_ptr->codecCtx->flags = flags;
}

int CodecContext::flags() const
{
    return d_ptr->codecCtx->flags;
}

bool CodecContext::open()
{
    Q_ASSERT(d_ptr->codecCtx != nullptr);
    Q_ASSERT(d_ptr->codec != nullptr);
    int ret = avcodec_open2(d_ptr->codecCtx, d_ptr->codec, NULL);
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

bool CodecContext::receiveFrame(Frame *frame)
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

int CodecContext::width() const
{
    return d_ptr->codecCtx->width;
}

int CodecContext::height() const
{
    return d_ptr->codecCtx->height;
}

AVMediaType CodecContext::mediaType() const
{
    return d_ptr->codecCtx->codec_type;
}

QString CodecContext::mediaTypeString() const
{
    return av_get_media_type_string(d_ptr->codecCtx->codec_type);
}

bool CodecContext::isDecoder() const
{
    Q_ASSERT(d_ptr->codec != nullptr);
    return av_codec_is_decoder(d_ptr->codec) != 0;
}

void CodecContext::flush()
{
    avcodec_flush_buffers(d_ptr->codecCtx);
}

AVError CodecContext::avError()
{
    return d_ptr->error;
}

AVCodec *CodecContext::codec()
{
    return d_ptr->codec;
}

void CodecContext::setError(int errorCode)
{
    d_ptr->error.setError(errorCode);
    emit error(d_ptr->error);
}

} // namespace Ffmpeg
