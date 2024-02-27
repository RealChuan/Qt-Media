#include "encodecontext.hpp"
#include "audioframeconverter.h"
#include "avcontextinfo.h"
#include "codeccontext.h"
#include "mediainfo.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

EncodeContext::EncodeContext(AVStream *stream, AVContextInfo *info)
{
    auto *codecpar = stream->codecpar;
    auto *avCodecContext = info->codecCtx()->avCodecCtx();
    const auto *codec = avCodecContext->codec;
    streamIndex = stream->index;
    mediaType = avCodecContext->codec_type;
    minBitrate = avCodecContext->rc_min_rate;
    maxBitrate = avCodecContext->rc_max_rate;
    bitrate = avCodecContext->bit_rate;
    if (bitrate <= 0) {
        bitrate = codecpar->bit_rate;
    }
    if (bitrate <= 0 && mediaType == AVMEDIA_TYPE_AUDIO) {
        bitrate = 512000;
    }
    size = {avCodecContext->width, avCodecContext->height};
    if (!size.isValid()) {
        size = {codecpar->width, codecpar->height};
    }
    sampleRate = avCodecContext->sample_rate;
    if (sampleRate <= 0) {
        sampleRate = codecpar->sample_rate;
    }

    if (!setEncoderName(QString::fromUtf8(codec->name))) {
        setEncoderName(codec->id);
    }
    setChannel(static_cast<AVChannel>(avCodecContext->ch_layout.u.mask));
    setProfile(avCodecContext->profile);

    sourceInfo = StreamInfo(stream).info();
}

auto EncodeContext::setEncoderName(AVCodecID codecId) -> bool
{
    QScopedPointer<AVContextInfo> contextInfoPtr(new AVContextInfo);
    if (!contextInfoPtr->initEncoder(codecId)) {
        return false;
    }
    init(contextInfoPtr.data());
    return true;
}

auto EncodeContext::setEncoderName(const QString &name) -> bool
{
    QScopedPointer<AVContextInfo> contextInfoPtr(new AVContextInfo);
    if (!contextInfoPtr->initEncoder(name)) {
        return false;
    }
    init(contextInfoPtr.data());
    return true;
}

void EncodeContext::setChannel(AVChannel channel)
{
    auto index = chLayouts.indexOf({channel, {}});
    if (index >= 0) {
        m_chLayout = chLayouts[index];
    }
}

void EncodeContext::setProfile(int profile)
{
    for (const auto &prof : std::as_const(profiles)) {
        if (prof.profile == profile) {
            m_profile = prof;
            return;
        }
    }
}

void EncodeContext::init(AVContextInfo *info)
{
    auto *codecContext = info->codecCtx();
    const auto *codec = codecContext->avCodecCtx()->codec;
    m_codecInfo.codecId = codec->id;
    m_codecInfo.name = QString::fromUtf8(codec->name);
    m_codecInfo.longName = QString::fromUtf8(codec->long_name);
    m_codecInfo.displayName = QString("%1 (%2)").arg(m_codecInfo.longName, m_codecInfo.name);
    qInfo() << "init EncodeContext: " << m_codecInfo.displayName;
    profiles = codecContext->supportedProfiles();
    if (!profiles.isEmpty()) {
        m_profile = profiles.first();
    }

    chLayouts = Ffmpeg::getChLayouts(codecContext->supportedChLayouts());
    auto index = chLayouts.indexOf({static_cast<AVChannel>(AV_CH_LAYOUT_STEREO), {}});
    if (index >= 0) {
        m_chLayout = chLayouts[index];
    } else if (!chLayouts.isEmpty()) {
        m_chLayout = chLayouts.first();
    }

    sampleRates = codecContext->supportedSampleRates();
    if (sampleRates.isEmpty()) {
        sampleRates = {96000, 48000, 44100, 32000, 22050, 16000, 11025, 8000};
    }
    if (!sampleRates.contains(sampleRate)) {
        sampleRate = sampleRates.first();
    }

    if (codec->type == AVMEDIA_TYPE_AUDIO) {
        qDebug() << "supported sample rates: " << sampleRates;
    }
}

} // namespace Ffmpeg
