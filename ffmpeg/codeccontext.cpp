#include "codeccontext.h"
#include "averror.h"
#include "frame.hpp"
#include "packet.h"
#include "subtitle.h"

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
}

namespace Ffmpeg {

struct CodecContext::CodecContextPrivate
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

void CodecContext::setDecodeThreadCount(int threadCount)
{
    Q_ASSERT(d_ptr->codecCtx != nullptr);
    d_ptr->codecCtx->thread_count = threadCount;
}

bool CodecContext::open(AVCodec *codec)
{
    Q_ASSERT(d_ptr->codecCtx != nullptr);
    int ret = avcodec_open2(d_ptr->codecCtx, codec, NULL);
    if (ret < 0) {
        setError(ret);
        return false;
    }
    qInfo() << d_ptr->codecCtx->framerate.num;
    qInfo() << "Width: " << d_ptr->codecCtx->width << " Height: " << d_ptr->codecCtx->height;
    qInfo() << "Channels: " << d_ptr->codecCtx->channels;
    qInfo() << "sample_fmt: " << av_get_sample_fmt_name(d_ptr->codecCtx->sample_fmt);
    qInfo() << "sample_rate: " << d_ptr->codecCtx->sample_rate;
    qInfo() << "channel_layout: " << d_ptr->codecCtx->channel_layout;
    qInfo() << "Color Range: " << av_color_range_name(d_ptr->codecCtx->color_range);
    qInfo() << "Color Primaries: " << av_color_primaries_name(d_ptr->codecCtx->color_primaries);
    qInfo() << "Color TransferCharacteristic: "
            << av_color_transfer_name(d_ptr->codecCtx->color_trc);
    qInfo() << "Color Space: " << av_color_space_name(d_ptr->codecCtx->colorspace);
    qInfo() << "ChromaLocation: "
            << av_chroma_location_name(d_ptr->codecCtx->chroma_sample_location);
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
