#include "codeccontext.h"
#include "averror.h"
#include "frame.hpp"
#include "packet.h"
#include "subtitle.h"

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

namespace Ffmpeg {

class CodecContext::CodecContextPrivate
{
public:
    CodecContextPrivate() {}
    ~CodecContextPrivate() { avcodec_free_context(&codecCtx); }

    void init()
    {
        Q_ASSERT(nullptr != codec);
        if (codec->supported_framerates) {
            for (int i = 0; i < INT_MAX; i++) {
                auto framerate = codec->supported_framerates[i];
                if (framerate.num == 0 && framerate.den == 0) {
                    break;
                }
                supported_framerates.append(framerate);
            }
        }
        if (codec->pix_fmts) {
            for (int i = 0; i < INT_MAX; i++) {
                auto pix_fmt = codec->pix_fmts[i];
                if (pix_fmt == -1) {
                    break;
                }
                supported_pix_fmts.append(pix_fmt);
            }
        }
        if (codec->supported_samplerates) {
            for (int i = 0; i < INT_MAX; i++) {
                auto samplerate = codec->supported_samplerates[i];
                if (samplerate == 0) {
                    break;
                }
                supported_samplerates.append(samplerate);
            }
        }
        if (codec->sample_fmts) {
            for (int i = 0; i < INT_MAX; i++) {
                auto sample_fmt = codec->sample_fmts[i];
                if (sample_fmt == -1) {
                    break;
                }
                supported_sample_fmts.append(sample_fmt);
            }
        }
        if (codec->channel_layouts) {
            for (int i = 0; i < INT_MAX; i++) {
                auto channel_layout = codec->channel_layouts[i];
                if (channel_layout == 0) {
                    break;
                }
                supported_channel_layouts.append(channel_layout);
            }
        }
    }

    const AVCodec *codec = nullptr;
    AVCodecContext *codecCtx = nullptr; //解码器上下文

    QVector<AVRational> supported_framerates{};
    QVector<AVPixelFormat> supported_pix_fmts{};
    QVector<int> supported_samplerates{};
    QVector<AVSampleFormat> supported_sample_fmts{};
    QVector<uint64_t> supported_channel_layouts{};

    AVError error;
};

CodecContext::CodecContext(const AVCodec *codec, QObject *parent)
    : QObject(parent)
    , d_ptr(new CodecContextPrivate)
{
    d_ptr->codec = codec;
    d_ptr->codecCtx = avcodec_alloc_context3(codec);
    d_ptr->init();
    Q_ASSERT(d_ptr->codecCtx != nullptr);
}

CodecContext::~CodecContext() {}

void CodecContext::copyToCodecParameters(CodecContext *dst)
{
    auto dstCodecCtx = dst->d_ptr->codecCtx;
    // quality
    dstCodecCtx->bit_rate = d_ptr->codecCtx->bit_rate;
    dstCodecCtx->rc_max_rate = d_ptr->codecCtx->rc_max_rate;
    dstCodecCtx->rc_min_rate = d_ptr->codecCtx->rc_min_rate;
    dstCodecCtx->qmin = d_ptr->codecCtx->qmin;
    dstCodecCtx->qmax = d_ptr->codecCtx->qmax; // smaller -> better
    dstCodecCtx->qblur = d_ptr->codecCtx->qblur;
    dstCodecCtx->qcompress = d_ptr->codecCtx->qcompress;
    dstCodecCtx->max_qdiff = d_ptr->codecCtx->max_qdiff;
    dstCodecCtx->bits_per_raw_sample = d_ptr->codecCtx->bits_per_raw_sample;
    dstCodecCtx->compression_level = d_ptr->codecCtx->compression_level;

    switch (mediaType()) {
    case AVMEDIA_TYPE_AUDIO:
        dst->setSampleRate(d_ptr->codecCtx->sample_rate);
        dst->setChannelLayout(d_ptr->codecCtx->channel_layout);
        dstCodecCtx->channels = av_get_channel_layout_nb_channels(dstCodecCtx->channel_layout);
        /* take first format from list of supported formats */
        dst->setSampleFmt(d_ptr->codec->sample_fmts[0]);
        dstCodecCtx->time_base = AVRational{1, dstCodecCtx->sample_rate};
        break;
    case AVMEDIA_TYPE_VIDEO:
        dstCodecCtx->height = d_ptr->codecCtx->height;
        dstCodecCtx->width = d_ptr->codecCtx->width;
        dstCodecCtx->sample_aspect_ratio = d_ptr->codecCtx->sample_aspect_ratio;
        /* take first format from list of supported formats */
        dst->setPixfmt(d_ptr->codec->pix_fmts ? d_ptr->codec->pix_fmts[0]
                                              : d_ptr->codecCtx->pix_fmt);
        /* video time_base can be set to whatever is handy and supported by encoder */
        dstCodecCtx->time_base = av_inv_q(d_ptr->codecCtx->framerate);
        dstCodecCtx->color_primaries = d_ptr->codecCtx->color_primaries;
        dstCodecCtx->color_trc = d_ptr->codecCtx->color_trc;
        dstCodecCtx->colorspace = d_ptr->codecCtx->colorspace;
        dstCodecCtx->color_range = d_ptr->codecCtx->color_range;
        dstCodecCtx->chroma_sample_location = d_ptr->codecCtx->chroma_sample_location;
        dstCodecCtx->slices = d_ptr->codecCtx->slices;
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

void CodecContext::setPixfmt(AVPixelFormat pixfmt)
{
    if (d_ptr->supported_pix_fmts.isEmpty() || d_ptr->supported_pix_fmts.contains(pixfmt)) {
        d_ptr->codecCtx->pix_fmt = pixfmt;
        return;
    }
    d_ptr->codecCtx->pix_fmt = d_ptr->supported_pix_fmts.first();
}

AVPixelFormat CodecContext::pixfmt() const
{
    return d_ptr->codecCtx->pix_fmt;
}

void CodecContext::setSampleRate(int sampleRate)
{
    if (d_ptr->supported_samplerates.isEmpty()
        || d_ptr->supported_samplerates.contains(sampleRate)) {
        d_ptr->codecCtx->sample_rate = sampleRate;
        return;
    }
    d_ptr->codecCtx->sample_rate = d_ptr->supported_samplerates.first();
}

int CodecContext::sampleRate() const
{
    return d_ptr->codecCtx->sample_rate;
}

void CodecContext::setSampleFmt(AVSampleFormat sampleFmt)
{
    if (d_ptr->supported_sample_fmts.isEmpty() || d_ptr->supported_sample_fmts.contains(sampleFmt)) {
        d_ptr->codecCtx->sample_fmt = sampleFmt;
        return;
    }
    d_ptr->codecCtx->sample_fmt = d_ptr->supported_sample_fmts.first();
}

AVSampleFormat CodecContext::sampleFmt() const
{
    return d_ptr->codecCtx->sample_fmt;
}

void CodecContext::setChannelLayout(uint64_t channelLayout)
{
    if (d_ptr->supported_channel_layouts.isEmpty()
        || d_ptr->supported_channel_layouts.contains(channelLayout)) {
        d_ptr->codecCtx->channel_layout = channelLayout;
        return;
    }
    d_ptr->codecCtx->channel_layout = d_ptr->supported_channel_layouts.first();
}

uint64_t CodecContext::channelLayout() const
{
    return d_ptr->codecCtx->channel_layout;
}

void CodecContext::setSize(const QSize &size)
{
    if (!size.isValid()) {
        return;
    }
    d_ptr->codecCtx->width = size.width();
    d_ptr->codecCtx->height = size.height();
    // fix me to improve image quality
    auto bit_rate = d_ptr->codecCtx->width * d_ptr->codecCtx->height * 4;
    d_ptr->codecCtx->bit_rate = bit_rate;
    d_ptr->codecCtx->rc_min_rate = bit_rate;
    d_ptr->codecCtx->rc_max_rate = bit_rate;
}

QSize CodecContext::size() const
{
    return {d_ptr->codecCtx->width, d_ptr->codecCtx->height};
}

void CodecContext::setQuailty(int quailty)
{
    Q_ASSERT(quailty >= d_ptr->codecCtx->qmin);
    d_ptr->codecCtx->flags |= AV_CODEC_FLAG_QSCALE;
    d_ptr->codecCtx->global_quality = FF_QP2LAMBDA * quailty;
}

QPair<int, int> CodecContext::quantizer() const
{
    return {d_ptr->codecCtx->qmin, d_ptr->codecCtx->qmax};
}

QVector<AVPixelFormat> CodecContext::supportPixFmts() const
{
    return d_ptr->supported_pix_fmts;
}

QVector<AVSampleFormat> CodecContext::supportSampleFmts() const
{
    return d_ptr->supported_sample_fmts;
}

void CodecContext::setMinBitrate(int64_t bitrate)
{
    Q_ASSERT(bitrate > 0);
    d_ptr->codecCtx->rc_min_rate = bitrate;
}

void CodecContext::setMaxBitrate(int64_t bitrate)
{
    qDebug() << bitrate << d_ptr->codecCtx->rc_min_rate;
    Q_ASSERT(bitrate >= d_ptr->codecCtx->rc_min_rate);
    d_ptr->codecCtx->rc_max_rate = bitrate;
    d_ptr->codecCtx->bit_rate = bitrate;
}

void CodecContext::setCrf(int crf)
{
    Q_ASSERT(crf >= 0 && crf <= 51);
    av_opt_set_int(d_ptr->codecCtx, "crf", crf, AV_OPT_SEARCH_CHILDREN);
}

void CodecContext::setPreset(const QString &preset)
{
    av_opt_set(d_ptr->codecCtx, "preset", preset.toLocal8Bit().constData(), AV_OPT_SEARCH_CHILDREN);
}

void CodecContext::setTune(const QString &tune)
{
    av_opt_set(d_ptr->codecCtx, "tune", tune.toLocal8Bit().constData(), AV_OPT_SEARCH_CHILDREN);
}

void CodecContext::setProfile(const QString &profile)
{
    av_opt_set(d_ptr->codecCtx,
               "profile",
               profile.toLocal8Bit().constData(),
               AV_OPT_SEARCH_CHILDREN);
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

bool CodecContext::sendFrame(Frame *frame)
{
    auto ret = avcodec_send_frame(d_ptr->codecCtx, frame->avFrame());
    if (ret < 0) {
        setError(ret);
        return false;
    }
    return true;
}

bool CodecContext::receivePacket(Packet *packet)
{
    auto ret = avcodec_receive_packet(d_ptr->codecCtx, packet->avPacket());
    if (ret < 0) {
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

const AVCodec *CodecContext::codec()
{
    return d_ptr->codec;
}

void CodecContext::setError(int errorCode)
{
    d_ptr->error.setError(errorCode);
    emit error(d_ptr->error);
}

} // namespace Ffmpeg
