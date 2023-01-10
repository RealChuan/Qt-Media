#include "audioframeconverter.h"
#include "averrormanager.hpp"
#include "codeccontext.h"
#include "frame.hpp"

#include <QAudioDevice>
#include <QDebug>
#include <QMediaDevices>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

namespace Ffmpeg {

qint64 getChannaLayoutFromChannalCount(int nb_channals)
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

qint64 getChannelLayout(QAudioFormat::ChannelConfig channelConfig)
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
QAudioFormat::ChannelConfig getChannelConfig(qint64 channalLayout)
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

AVSampleFormat getAVSampleFormat(QAudioFormat::SampleFormat format)
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

QAudioFormat::SampleFormat getSampleFormat(AVSampleFormat format)
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
    AudioFrameConverterPrivate() {}

    void setError(int errorCode) { AVErrorManager::instance()->setErrorCode(errorCode); }

    SwrContext *swrContext;
    QAudioFormat format;
    AVSampleFormat avSampleFormat;
};

AudioFrameConverter::AudioFrameConverter(CodecContext *codecCtx, QAudioFormat &format)
    : d_ptr(new AudioFrameConverterPrivate)
{
    d_ptr->format = format;
    d_ptr->avSampleFormat = getAVSampleFormat(d_ptr->format.sampleFormat());
    auto channelLayout = getChannelLayout(d_ptr->format.channelConfig());
    auto ctx = codecCtx->avCodecCtx();
    auto channel_layout = ctx->channel_layout;
    if (channel_layout <= 0) {
        av_get_default_channel_layout(ctx->channels);
    }
    d_ptr->swrContext = swr_alloc_set_opts(nullptr,
                                           channelLayout,
                                           d_ptr->avSampleFormat,
                                           d_ptr->format.sampleRate(),
                                           channel_layout,
                                           ctx->sample_fmt,
                                           ctx->sample_rate,
                                           0,
                                           nullptr);

    int ret = swr_init(d_ptr->swrContext);
    if (ret < 0) {
        d_ptr->setError(ret);
    }
    Q_ASSERT(d_ptr->swrContext != nullptr);
}

AudioFrameConverter::~AudioFrameConverter()
{
    Q_ASSERT(d_ptr->swrContext != nullptr);
    swr_free(&d_ptr->swrContext);
}

QByteArray AudioFrameConverter::convert(Frame *frame)
{
    int size = av_samples_get_buffer_size(nullptr,
                                          d_ptr->format.channelCount(),
                                          frame->avFrame()->nb_samples,
                                          d_ptr->avSampleFormat,
                                          0);

    QByteArray data(size, Qt::Uninitialized);
    quint8 *bufPointer[] = {(quint8 *) data.data()};

    int len = swr_convert(d_ptr->swrContext,
                          bufPointer,
                          frame->avFrame()->nb_samples,
                          const_cast<const uint8_t **>(frame->avFrame()->data),
                          frame->avFrame()->nb_samples);
    if (len <= 0) {
        data.clear();
        d_ptr->setError(len);
    }

    return data;
}

QAudioFormat getAudioFormatFromCodecCtx(CodecContext *codecCtx, int &sampleSize)
{
    auto ctx = codecCtx->avCodecCtx();
    QAudioFormat autioFormat;
    //autioFormat.setCodec("audio/pcm");
    autioFormat.setSampleRate(ctx->sample_rate);
    autioFormat.setChannelCount(ctx->channels);
    //autioFormat.setByteOrder(QAudioFormat::LittleEndian);

    if (ctx->channel_layout <= 0) {
        ctx->channel_layout = getChannaLayoutFromChannalCount(ctx->channels);
    }
    auto channelConfig = getChannelConfig(ctx->channel_layout);
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
        qWarning() << autioFormat << " is not supported by backend, cannot play audio.";
        Q_ASSERT(48000 >= audioDevice.minimumSampleRate()
                 && 48000 <= audioDevice.maximumSampleRate());
        Q_ASSERT(2 >= audioDevice.minimumChannelCount() && 2 <= audioDevice.maximumChannelCount());
        autioFormat.setSampleRate(48000);
        autioFormat.setChannelCount(2);
        autioFormat.setChannelConfig(QAudioFormat::ChannelConfigStereo);
        autioFormat.setSampleFormat(QAudioFormat::Int16);
        sampleSize = 8 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    }

    qInfo() << "Current Audio parameters:" << ctx->sample_rate << ctx->channels
            << ctx->channel_layout << ctx->sample_fmt;
    qInfo() << autioFormat << autioFormat.channelConfig();

    return autioFormat;
}

} // namespace Ffmpeg
