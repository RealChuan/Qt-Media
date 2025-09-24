#include "audioframeconverter.h"
#include "averrormanager.hpp"
#include "codeccontext.h"

#include <QAudioDevice>
#include <QDebug>
#include <QMediaDevices>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

namespace Ffmpeg {

static auto getChannaLayoutFromChannalCount(int nb_channals) -> qint64
{
    qint64 channalLayout = 0;
    switch (nb_channals) {
    case 1: channalLayout = AV_CH_LAYOUT_MONO; break;
    case 2: channalLayout = AV_CH_LAYOUT_STEREO; break;
    case 3: channalLayout = AV_CH_LAYOUT_2POINT1; break;
    case 4: channalLayout = AV_CH_LAYOUT_3POINT1; break;
    case 5: channalLayout = AV_CH_LAYOUT_5POINT0_BACK; break;
    case 6: channalLayout = AV_CH_LAYOUT_5POINT1_BACK; break;
    case 7: channalLayout = AV_CH_LAYOUT_7POINT0; break;
    case 8: channalLayout = AV_CH_LAYOUT_7POINT1; break;
    case 16: channalLayout = AV_CH_LAYOUT_HEXADECAGONAL; break;
    case 24: channalLayout = AV_CH_LAYOUT_22POINT2; break;
    default: break;
    }
    return channalLayout;
}

static auto getChannelLayout(QAudioFormat::ChannelConfig channelConfig) -> qint64
{
    qint64 channal = 0;
    switch (channelConfig) {
    case QAudioFormat::ChannelConfigUnknown: break;
    case QAudioFormat::ChannelConfigMono: channal = AV_CH_LAYOUT_MONO; break;
    case QAudioFormat::ChannelConfigStereo: channal = AV_CH_LAYOUT_STEREO; break;
    case QAudioFormat::ChannelConfig2Dot1: channal = AV_CH_LAYOUT_2POINT1; break;
    case QAudioFormat::ChannelConfig3Dot0: channal = AV_CH_LAYOUT_SURROUND; break;
    case QAudioFormat::ChannelConfig3Dot1: channal = AV_CH_LAYOUT_3POINT1; break;
    case QAudioFormat::ChannelConfigSurround5Dot0: channal = AV_CH_LAYOUT_5POINT0_BACK; break;
    case QAudioFormat::ChannelConfigSurround5Dot1: channal = AV_CH_LAYOUT_5POINT1_BACK; break;
    case QAudioFormat::ChannelConfigSurround7Dot0: channal = AV_CH_LAYOUT_7POINT0; break;
    case QAudioFormat::ChannelConfigSurround7Dot1: channal = AV_CH_LAYOUT_7POINT1; break;
    default: break;
    }

    return channal;
}
static auto getChannelConfig(qint64 channalLayout) -> QAudioFormat::ChannelConfig
{
    auto config = QAudioFormat::ChannelConfigUnknown;
    switch (channalLayout) {
    case AV_CH_LAYOUT_MONO: config = QAudioFormat::ChannelConfigMono; break;
    case AV_CH_LAYOUT_STEREO: config = QAudioFormat::ChannelConfigStereo; break;
    case AV_CH_LAYOUT_2POINT1: config = QAudioFormat::ChannelConfig2Dot1; break;
    case AV_CH_LAYOUT_SURROUND: config = QAudioFormat::ChannelConfig3Dot0; break;
    case AV_CH_LAYOUT_3POINT1: config = QAudioFormat::ChannelConfig3Dot1; break;
    case AV_CH_LAYOUT_5POINT0_BACK: config = QAudioFormat::ChannelConfigSurround5Dot0; break;
    case AV_CH_LAYOUT_5POINT1_BACK: config = QAudioFormat::ChannelConfigSurround5Dot1; break;
    case AV_CH_LAYOUT_7POINT0: config = QAudioFormat::ChannelConfigSurround7Dot0; break;
    case AV_CH_LAYOUT_7POINT1: config = QAudioFormat::ChannelConfigSurround7Dot1; break;
    default: break;
    }

    return config;
}

static auto getAVSampleFormat(QAudioFormat::SampleFormat format) -> AVSampleFormat
{
    auto sampleFormat = AV_SAMPLE_FMT_NONE;
    switch (format) {
    case QAudioFormat::Unknown: sampleFormat = AV_SAMPLE_FMT_NONE; break;
    case QAudioFormat::UInt8: sampleFormat = AV_SAMPLE_FMT_U8; break;
    case QAudioFormat::Int16: sampleFormat = AV_SAMPLE_FMT_S16; break;
    case QAudioFormat::Int32: sampleFormat = AV_SAMPLE_FMT_S32; break;
    case QAudioFormat::Float: sampleFormat = AV_SAMPLE_FMT_FLT; break;
    default: break;
    }
    return sampleFormat;
}

static auto getSampleFormat(AVSampleFormat format) -> QAudioFormat::SampleFormat
{
    auto sampleFormat = QAudioFormat::Unknown;
    switch (format) {
    case AV_SAMPLE_FMT_NONE: sampleFormat = QAudioFormat::Unknown; break;
    case AV_SAMPLE_FMT_U8: sampleFormat = QAudioFormat::UInt8; break;
    case AV_SAMPLE_FMT_S16: sampleFormat = QAudioFormat::Int16; break;
    case AV_SAMPLE_FMT_S32: sampleFormat = QAudioFormat::Int32; break;
    case AV_SAMPLE_FMT_FLT: sampleFormat = QAudioFormat::Float; break;
    default: break;
    }
    return sampleFormat;
}

class AudioFrameConverter::AudioFrameConverterPrivate
{
public:
    explicit AudioFrameConverterPrivate(AudioFrameConverter *q)
        : q_ptr(q)
    {}

    AudioFrameConverter *q_ptr;

    SwrContext *swrContext = nullptr;
    QAudioFormat format;
    AVSampleFormat avSampleFormat;
};

AudioFrameConverter::AudioFrameConverter(CodecContext *codecCtx,
                                         const QAudioFormat &format,
                                         QObject *parent)
    : QObject(parent)
    , d_ptr(new AudioFrameConverterPrivate(this))
{
    d_ptr->format = format;
    d_ptr->avSampleFormat = getAVSampleFormat(d_ptr->format.sampleFormat());
    auto *avCodecCtx = codecCtx->avCodecCtx();
    AVChannelLayout chLayout = {AV_CHANNEL_ORDER_UNSPEC};
    av_channel_layout_default(&chLayout, d_ptr->format.channelCount());
    // av_channel_layout_from_mask(&chLayout, getChannelLayout(d_ptr->format.channelConfig()));
    auto ret = swr_alloc_set_opts2(&d_ptr->swrContext,
                                   &chLayout,
                                   d_ptr->avSampleFormat,
                                   d_ptr->format.sampleRate(),
                                   &avCodecCtx->ch_layout,
                                   avCodecCtx->sample_fmt,
                                   avCodecCtx->sample_rate,
                                   0,
                                   nullptr);
    if (ret != 0) {
        SET_ERROR_CODE(ret);
    }
    ret = swr_init(d_ptr->swrContext);
    if (ret < 0) {
        SET_ERROR_CODE(ret);
    }
    Q_ASSERT(d_ptr->swrContext != nullptr);
}

AudioFrameConverter::~AudioFrameConverter()
{
    Q_ASSERT(d_ptr->swrContext != nullptr);
    swr_free(&d_ptr->swrContext);
}

auto AudioFrameConverter::convert(const FramePtr &framePtr) -> QByteArray
{
    auto *avFrame = framePtr->avFrame();
    auto nb_samples = avFrame->nb_samples;
    auto out_count = static_cast<int64_t>(nb_samples) * d_ptr->format.sampleRate()
                         / avFrame->sample_rate
                     + 256; // 256 copy from ffplay
    auto size = av_samples_get_buffer_size(nullptr,
                                           d_ptr->format.channelCount(),
                                           out_count,
                                           d_ptr->avSampleFormat,
                                           0);

    QByteArray data(size, Qt::Uninitialized);
    quint8 *bufPointer[] = {reinterpret_cast<quint8 *>(data.data())};
    auto len = swr_convert(d_ptr->swrContext,
                           bufPointer,
                           out_count,
                           const_cast<const uint8_t **>(avFrame->extended_data),
                           nb_samples);
    if (len <= 0) {
        data.clear();
        SET_ERROR_CODE(len);
    } else if (len == out_count) {
        qWarning() << "audio buffer is probably too small";
    }
    size = len * d_ptr->format.channelCount() * av_get_bytes_per_sample(d_ptr->avSampleFormat);
    data.truncate(size);

    return data;
}

auto getAudioFormatFromCodecCtx(CodecContext *codecCtx, int &sampleSize) -> QAudioFormat
{
    auto *ctx = codecCtx->avCodecCtx();
    QAudioFormat autioFormat;
    //autioFormat.setCodec("audio/pcm");
    autioFormat.setSampleRate(ctx->sample_rate);
    autioFormat.setChannelCount(ctx->ch_layout.nb_channels);
    //autioFormat.setByteOrder(QAudioFormat::LittleEndian);

    if (ctx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
        av_channel_layout_default(&ctx->ch_layout, ctx->ch_layout.nb_channels);
    }
    auto channelConfig = getChannelConfig(ctx->ch_layout.u.mask);
    if (channelConfig == QAudioFormat::ChannelConfigUnknown) {
        channelConfig = QAudioFormat::ChannelConfigStereo;
    }
    autioFormat.setChannelConfig(channelConfig);

    auto sampleFormat = getSampleFormat(ctx->sample_fmt);
    sampleSize = 8 * av_get_bytes_per_sample(ctx->sample_fmt);
    if (sampleFormat == QAudioFormat::Unknown) {
        sampleFormat = QAudioFormat::Int16;
        sampleSize = 8 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    }
    autioFormat.setSampleFormat(sampleFormat);

    QAudioDevice audioDevice(QMediaDevices::defaultAudioOutput());
    qInfo() << "AudioDevice Support SampleFormats" << audioDevice.supportedSampleFormats();
    qInfo() << "AudioDevice Support SampleRate" << audioDevice.minimumSampleRate() << "~"
            << audioDevice.maximumSampleRate();
    qInfo() << "AudioDevice Support ChannelCount" << audioDevice.minimumChannelCount() << "~"
            << audioDevice.maximumChannelCount();
    if (!audioDevice.isFormatSupported(autioFormat)) {
        autioFormat = audioDevice.preferredFormat();
        sampleSize = 8 * av_get_bytes_per_sample(getAVSampleFormat(autioFormat.sampleFormat()));
        qInfo() << "Use preferred audio parameters: " << autioFormat;
    }
    qInfo() << "Current Audio parameters:" << ctx->sample_rate << ctx->ch_layout.nb_channels
            << ctx->ch_layout.u.mask << ctx->sample_fmt;
    qInfo() << autioFormat << autioFormat.channelConfig();

    return autioFormat;
}

auto getAVChannelLayoutDescribe(const AVChannelLayout &chLayout) -> QString
{
    char buf[64] = {0};
    av_channel_layout_describe(&chLayout, buf, sizeof(buf));
    return QString::fromUtf8(buf);
}

} // namespace Ffmpeg
