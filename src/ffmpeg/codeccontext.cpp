#include "codeccontext.h"
#include "averrormanager.hpp"
#include "encodecontext.hpp"
#include "ffmpegutils.hpp"
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
    explicit CodecContextPrivate(CodecContext *q)
        : q_ptr(q)
    {}
    ~CodecContextPrivate()
    {
        av_dict_free(&encodeOptions);
        avcodec_free_context(&codecCtx);
    }

    void init()
    {
        const auto *codec = codecCtx->codec;

        int nb_frame_rates;
        const void *frame_rates = nullptr;
        avcodec_get_supported_config(nullptr,
                                     codec,
                                     AV_CODEC_CONFIG_FRAME_RATE,
                                     0,
                                     &frame_rates,
                                     &nb_frame_rates);

        if (nb_frame_rates > 0) {
            const auto *codec_frame_rates = static_cast<const AVRational *>(frame_rates);
            for (int i = 0; i < nb_frame_rates; ++i) {
                auto frame_rate = codec_frame_rates[i];
                supported_framerates.append(frame_rate);
            }
        }

        int nb_pix_fmts;
        const void *pix_fmts = nullptr;
        avcodec_get_supported_config(nullptr,
                                     codec,
                                     AV_CODEC_CONFIG_PIX_FORMAT,
                                     0,
                                     &pix_fmts,
                                     &nb_pix_fmts);

        if (nb_pix_fmts > 0) {
            const auto *codec_pix_fmts = static_cast<const AVPixelFormat *>(pix_fmts);
            for (int i = 0; i < nb_pix_fmts; ++i) {
                auto pix_fmt = codec_pix_fmts[i];
                supported_pix_fmts.append(pix_fmt);
            }
        }

        int nb_sample_rates;
        const void *sample_rates = nullptr;
        avcodec_get_supported_config(nullptr,
                                     codec,
                                     AV_CODEC_CONFIG_SAMPLE_RATE,
                                     0,
                                     &sample_rates,
                                     &nb_sample_rates);

        if (nb_sample_rates > 0) {
            const int *codec_sample_rates = static_cast<const int *>(sample_rates);
            for (int i = 0; i < nb_sample_rates; ++i) {
                auto sample_rate = codec_sample_rates[i];
                supported_samplerates.append(sample_rate);
            }
        }

        int nb_sample_fmts;
        const void *sample_fmts = nullptr;
        avcodec_get_supported_config(nullptr,
                                     codec,
                                     AV_CODEC_CONFIG_SAMPLE_FORMAT,
                                     0,
                                     &sample_fmts,
                                     &nb_sample_fmts);

        if (nb_sample_fmts > 0) {
            const auto *codec_sample_fmts = static_cast<const AVSampleFormat *>(sample_fmts);
            for (int i = 0; i < nb_sample_fmts; ++i) {
                auto sample_fmt = codec_sample_fmts[i];
                supported_sample_fmts.append(sample_fmt);
            }
        }

        int nb_ch_layouts;
        const void *ch_layouts = nullptr;
        avcodec_get_supported_config(nullptr,
                                     codec,
                                     AV_CODEC_CONFIG_CHANNEL_LAYOUT,
                                     0,
                                     &ch_layouts,
                                     &nb_ch_layouts);

        if (nb_ch_layouts > 0) {
            const auto *codec_ch_layouts = static_cast<const AVChannelLayout *>(ch_layouts);
            for (int i = 0; i < nb_ch_layouts; ++i) {
                auto ch_layout = codec_ch_layouts[i];
                supported_ch_layouts.append(ch_layout);
            }
        }

        const auto *profile = codec->profiles;
        if (profile != nullptr) {
            while (profile->profile != AV_PROFILE_UNKNOWN) {
                supported_profiles.append(*profile);
                profile++;
            }
        }
    }

    // code copy from HandBrake encavcodec.c
    void initVideoEncoderOptions(const EncodeContext &encodeContext)
    {
        double crf = encodeContext.crf;
        if (crf == EncodeLimit::invalid_crf) {
            return;
        }

        Q_ASSERT(codecCtx->codec_type == AVMEDIA_TYPE_VIDEO);
        Q_ASSERT(crf >= EncodeLimit::crf_min && crf <= EncodeLimit::crf_max);

        auto codecName = encodeContext.codecInfo().name;

        if (codecCtx->codec_id == AV_CODEC_ID_VP8 || codecCtx->codec_id == AV_CODEC_ID_VP9) {
            codecCtx->flags |= AV_CODEC_FLAG_QSCALE;
            codecCtx->global_quality = FF_QP2LAMBDA * crf + 0.5;
            auto quality = QString::number(crf, 'f', 2);
            av_dict_set(&encodeOptions, "crf", quality.toUtf8().data(), 0);
        } else if (codecName.contains("_nvenc")) { // nvidia编码器
            double adjustedQualityI = crf - 2;
            double adjustedQualityB = crf + 2;
            if (adjustedQualityB > EncodeLimit::crf_max) {
                adjustedQualityB = EncodeLimit::crf_max;
            }

            if (adjustedQualityI < EncodeLimit::crf_min) {
                adjustedQualityI = EncodeLimit::crf_min;
            }

            auto quality = QString::number(crf, 'f', 2);
            auto qualityI = QString::number(adjustedQualityI, 'f', 2);
            auto qualityB = QString::number(adjustedQualityB, 'f', 2);

            av_dict_set(&encodeOptions, "rc", "vbr", 0);
            av_dict_set(&encodeOptions, "cq", quality.toUtf8().data(), 0);

            // further Advanced Quality Settings in Constant Quality Mode
            av_dict_set(&encodeOptions, "init_qpP", quality.toUtf8().data(), 0);
            av_dict_set(&encodeOptions, "init_qpB", qualityB.toUtf8().data(), 0);
            av_dict_set(&encodeOptions, "init_qpI", qualityI.toUtf8().data(), 0);
        } else if (codecName.contains("_vce")) { // amd vce编码器
            int maxQuality = EncodeLimit::crf_max;
            double qualityOffsetThreshold = 8;
            double qualityOffsetP = 2;
            double qualityOffsetB;
            double adjustedQualityP;
            double adjustedQualityB;

            if (codecCtx->codec_id == AV_CODEC_ID_AV1) {
                maxQuality = 255;
                qualityOffsetThreshold = 32;
                qualityOffsetP = 8;
            }

            if (crf <= qualityOffsetThreshold) {
                qualityOffsetP = crf / qualityOffsetThreshold * qualityOffsetP;
            }
            qualityOffsetB = qualityOffsetP * 2;

            adjustedQualityP = crf + qualityOffsetP;
            adjustedQualityB = crf + qualityOffsetB;
            if (adjustedQualityP > maxQuality) {
                adjustedQualityP = maxQuality;
            }
            if (adjustedQualityB > maxQuality) {
                adjustedQualityB = maxQuality;
            }

            auto quality = QString::number(crf, 'f', 2);
            auto qualityP = QString::number(adjustedQualityP, 'f', 2);
            auto qualityB = QString::number(adjustedQualityB, 'f', 2);

            av_dict_set(&encodeOptions, "rc", "cqp", 0);

            av_dict_set(&encodeOptions, "qp_i", quality.toUtf8().data(), 0);
            av_dict_set(&encodeOptions, "qp_p", qualityP.toUtf8().data(), 0);
            // H.265 encoders do not support B frames
            if (codecCtx->codec_id != AV_CODEC_ID_H265) {
                av_dict_set(&encodeOptions, "qp_b", qualityB.toUtf8().data(), 0);
            }
        } else if (codecName.contains("_mf")) { // ffmpeg mf编码器
            // why quality bigger is better?
            // https://trac.ffmpeg.org/wiki/Encode/H.264
            int quality = qMin(100, static_cast<int>(crf * 2));
            av_dict_set(&encodeOptions, "rate_control", "quality", 0);
            av_dict_set(&encodeOptions, "quality", QString::number(quality).toUtf8().data(), 0);
            // av_dict_set(&encodeOptions, "hw_encoding", "1", 0);
        } else {
            codecCtx->flags |= AV_CODEC_FLAG_QSCALE;
            codecCtx->global_quality = FF_QP2LAMBDA * crf + 0.5;
        }
    }

    void initAudioEncoderOptions(const EncodeContext &encodeContext)
    {
        Q_ASSERT(codecCtx->codec_type == AVMEDIA_TYPE_AUDIO);

        codecCtx->global_quality = encodeContext.crf * FF_QP2LAMBDA;
        codecCtx->flags |= AV_CODEC_FLAG_QSCALE;
        if (encodeContext.codecInfo().name.contains("fdk")) {
            auto vbr = QString::asprintf("%.1d", encodeContext.crf);
            av_dict_set(&encodeOptions, "vbr", vbr.toUtf8().data(), 0);
        }
    }

    void printIgnoredOptions() const
    {
        AVDictionaryEntry *dict = nullptr;
        while ((dict = av_dict_get(encodeOptions, "", dict, AV_DICT_IGNORE_SUFFIX)) != nullptr) {
            qWarning() << "Unknown avcodec option: " << dict->key;
        }
    }

    CodecContext *q_ptr;

    AVCodecContext *codecCtx = nullptr; //解码器上下文

    QList<AVRational> supported_framerates{};
    QList<AVPixelFormat> supported_pix_fmts{};
    QList<int> supported_samplerates{};
    QList<AVSampleFormat> supported_sample_fmts{};
    QList<AVChannelLayout> supported_ch_layouts{};
    QList<AVProfile> supported_profiles{};

    AVDictionary *encodeOptions = nullptr;
};

CodecContext::CodecContext(const AVCodec *codec, QObject *parent)
    : QObject(parent)
    , d_ptr(new CodecContextPrivate(this))
{
    qInfo() << "AVCodec:" << codec->name << " long name:" << codec->long_name;
    d_ptr->codecCtx = avcodec_alloc_context3(codec);
    d_ptr->init();
    Q_ASSERT(d_ptr->codecCtx != nullptr);
}

CodecContext::~CodecContext() = default;

void CodecContext::copyToCodecParameters(CodecContext *dst)
{
    auto *dstCodecCtx = dst->d_ptr->codecCtx;
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
    dstCodecCtx->gop_size = d_ptr->codecCtx->gop_size;
    dstCodecCtx->global_quality = d_ptr->codecCtx->global_quality;
    dst->setProfile(d_ptr->codecCtx->profile);

    switch (d_ptr->codecCtx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        dst->setSampleRate(d_ptr->codecCtx->sample_rate);
        dst->setChLayout(d_ptr->codecCtx->ch_layout);
        /* take first format from list of supported formats */
        dst->setSampleFmt(d_ptr->supported_sample_fmts.isEmpty()
                              ? d_ptr->codecCtx->sample_fmt
                              : d_ptr->supported_sample_fmts.first());
        dstCodecCtx->time_base = AVRational{1, dstCodecCtx->sample_rate};
        break;
    case AVMEDIA_TYPE_VIDEO:
        dstCodecCtx->height = d_ptr->codecCtx->height;
        dstCodecCtx->width = d_ptr->codecCtx->width;
        dstCodecCtx->sample_aspect_ratio = d_ptr->codecCtx->sample_aspect_ratio;
        /* take first format from list of supported formats */
        dst->setPixfmt(d_ptr->supported_pix_fmts.isEmpty() ? d_ptr->codecCtx->pix_fmt
                                                           : d_ptr->supported_pix_fmts.first());
        dst->setFrameRate(d_ptr->codecCtx->framerate);
        /* video time_base can be set to whatever is handy and supported by encoder */
        dstCodecCtx->time_base = av_inv_q(dstCodecCtx->framerate);
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

auto CodecContext::avCodecCtx() -> AVCodecContext *
{
    return d_ptr->codecCtx;
}

auto CodecContext::setParameters(const AVCodecParameters *par) -> bool
{
    int ret = avcodec_parameters_to_context(d_ptr->codecCtx, par);
    ERROR_RETURN(ret)
}

auto CodecContext::supportedFrameRates() const -> QList<AVRational>
{
    return d_ptr->supported_framerates;
}

void CodecContext::setFrameRate(const AVRational &frameRate)
{
    if (d_ptr->supported_framerates.isEmpty()) {
        d_ptr->codecCtx->framerate = frameRate;
        return;
    }
    for (const auto &rate : std::as_const(d_ptr->supported_framerates)) {
        if (compareAVRational(rate, frameRate)) {
            d_ptr->codecCtx->framerate = rate;
            return;
        }
    }
    d_ptr->codecCtx->framerate = d_ptr->supported_framerates.first();
}

void CodecContext::setThreadCount(int threadCount)
{
    Q_ASSERT(d_ptr->codecCtx != nullptr);
    d_ptr->codecCtx->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
    d_ptr->codecCtx->thread_count = threadCount;
}

void CodecContext::setPixfmt(AVPixelFormat pixfmt)
{
    if (d_ptr->supported_pix_fmts.isEmpty() || d_ptr->supported_pix_fmts.contains(pixfmt)) {
        d_ptr->codecCtx->pix_fmt = pixfmt;
        return;
    }
    d_ptr->codecCtx->pix_fmt = d_ptr->supported_pix_fmts.first();
}

auto CodecContext::supportedSampleRates() const -> QList<int>
{
    return d_ptr->supported_samplerates;
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

void CodecContext::setSampleFmt(AVSampleFormat sampleFmt)
{
    if (d_ptr->supported_sample_fmts.isEmpty() || d_ptr->supported_sample_fmts.contains(sampleFmt)) {
        d_ptr->codecCtx->sample_fmt = sampleFmt;
        return;
    }
    d_ptr->codecCtx->sample_fmt = d_ptr->supported_sample_fmts.first();
}

auto CodecContext::supportedProfiles() const -> QList<AVProfile>
{
    return d_ptr->supported_profiles;
}

void CodecContext::setProfile(int profile)
{
    if (d_ptr->supported_profiles.isEmpty()) {
        d_ptr->codecCtx->profile = profile;
        return;
    }
    for (const auto &prof : std::as_const(d_ptr->supported_profiles)) {
        if (prof.profile == profile) {
            d_ptr->codecCtx->profile = profile;
            return;
        }
    }
    d_ptr->codecCtx->profile = d_ptr->supported_profiles.first().profile;
}

auto CodecContext::supportedChLayouts() const -> QList<AVChannelLayout>
{
    return d_ptr->supported_ch_layouts;
}

void CodecContext::setEncodeParameters(const EncodeContext &encodeContext)
{
    setThreadCount(encodeContext.threadCount);
    setProfile(encodeContext.profile().profile);

    d_ptr->codecCtx->rc_min_rate = encodeContext.minBitrate;
    d_ptr->codecCtx->rc_max_rate = encodeContext.maxBitrate;
    d_ptr->codecCtx->bit_rate = encodeContext.bitrate;

    switch (encodeContext.mediaType) {
    case AVMEDIA_TYPE_VIDEO:
        setSize(encodeContext.size);
        d_ptr->initVideoEncoderOptions(encodeContext);
        break;
    case AVMEDIA_TYPE_AUDIO: {
        AVChannelLayout chLayout = {AV_CHANNEL_ORDER_UNSPEC};
        av_channel_layout_from_mask(&chLayout,
                                    static_cast<uint64_t>(encodeContext.chLayout().channel));
        setChLayout(chLayout);
        setSampleRate(encodeContext.sampleRate);
        d_ptr->initAudioEncoderOptions(encodeContext);
    } break;
    default: break;
    }
    // av_opt_set(d_ptr->codecCtx->priv_data,
    //            "preset",
    //            encodeContext.preset.toLocal8Bit().constData(),
    //            AV_OPT_SEARCH_CHILDREN);
    // av_opt_set(d_ptr->codecCtx->priv_data,
    //            "tune",
    //            encodeContext.tune.toLocal8Bit().constData(),
    //            AV_OPT_SEARCH_CHILDREN);
}

void CodecContext::setChLayout(const AVChannelLayout &chLayout)
{
    for (const auto &ch : std::as_const(d_ptr->supported_ch_layouts)) {
        if (0 == av_channel_layout_compare(&ch, &chLayout)) {
            av_channel_layout_copy(&d_ptr->codecCtx->ch_layout, &ch);
            return;
        }
    }
    if (d_ptr->supported_ch_layouts.isEmpty()) {
        av_channel_layout_from_mask(&d_ptr->codecCtx->ch_layout,
                                    static_cast<uint64_t>(AV_CH_LAYOUT_STEREO));
        return;
    }
    av_channel_layout_copy(&d_ptr->codecCtx->ch_layout, &d_ptr->supported_ch_layouts.first());
}

auto CodecContext::chLayout() const -> AVChannelLayout
{
    return d_ptr->codecCtx->ch_layout;
}

void CodecContext::setSize(const QSize &size)
{
    if (!size.isValid()) {
        return;
    }
    d_ptr->codecCtx->width = size.width();
    d_ptr->codecCtx->height = size.height();
}

auto CodecContext::size() const -> QSize
{
    return {d_ptr->codecCtx->width, d_ptr->codecCtx->height};
}

auto CodecContext::quantizer() const -> QPair<int, int>
{
    return {d_ptr->codecCtx->qmin, d_ptr->codecCtx->qmax};
}

auto CodecContext::supportedPixFmts() const -> QList<AVPixelFormat>
{
    return d_ptr->supported_pix_fmts;
}

auto CodecContext::supportedSampleFmts() const -> QList<AVSampleFormat>
{
    return d_ptr->supported_sample_fmts;
}

auto CodecContext::open() -> bool
{
    Q_ASSERT(d_ptr->codecCtx != nullptr);
    auto ret = avcodec_open2(d_ptr->codecCtx, nullptr, &d_ptr->encodeOptions);
    d_ptr->printIgnoredOptions();
    ERROR_RETURN(ret)
}

auto CodecContext::sendPacket(const PacketPtr &packetPtr) -> bool
{
    int ret = avcodec_send_packet(d_ptr->codecCtx, packetPtr->avPacket());
    AVERROR(EINVAL);
    ERROR_RETURN(ret)
}

auto CodecContext::receiveFrame(const FramePtr &framePtr) -> bool
{
    int ret = avcodec_receive_frame(d_ptr->codecCtx, framePtr->avFrame());
    if (ret >= 0) {
        auto *avFrame = framePtr->avFrame();
        avFrame->sample_aspect_ratio = d_ptr->codecCtx->sample_aspect_ratio;
        avFrame->sample_rate = d_ptr->codecCtx->sample_rate;
        avFrame->ch_layout = d_ptr->codecCtx->ch_layout;
        return true;
    }
    if (ret != AVERROR(EAGAIN)) { // Resource temporarily unavailable
        SET_ERROR_CODE(ret);
    }
    return false;
}

auto CodecContext::decodeSubtitle2(Subtitle *subtitle, const PacketPtr &packetPtr) -> bool
{
    int got_sub_ptr = 0;
    int ret = avcodec_decode_subtitle2(d_ptr->codecCtx,
                                       subtitle->avSubtitle(),
                                       &got_sub_ptr,
                                       packetPtr->avPacket());
    if (ret < 0 || got_sub_ptr <= 0) {
        SET_ERROR_CODE(ret);
        return false;
    }
    return true;
}

auto CodecContext::sendFrame(const FramePtr &framePtr) -> bool
{
    auto ret = avcodec_send_frame(d_ptr->codecCtx, framePtr->avFrame());
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return false;
    }
    ERROR_RETURN(ret)
}

auto CodecContext::receivePacket(const PacketPtr &packetPtr) -> bool
{
    auto ret = avcodec_receive_packet(d_ptr->codecCtx, packetPtr->avPacket());
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return false;
    }
    ERROR_RETURN(ret)
}

auto CodecContext::mediaTypeString() const -> QString
{
    return av_get_media_type_string(d_ptr->codecCtx->codec_type);
}

auto CodecContext::isDecoder() const -> bool
{
    return av_codec_is_decoder(d_ptr->codecCtx->codec) != 0;
}

void CodecContext::flush()
{
    avcodec_flush_buffers(d_ptr->codecCtx);
}

} // namespace Ffmpeg
