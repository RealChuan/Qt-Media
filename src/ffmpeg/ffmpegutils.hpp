#pragma once

#include "ffmepg_global.h"
#include "frame.hpp"

#include <QMetaType>
#include <QSize>

extern "C" {
#include <libavcodec/codec.h>
#include <libavutil/hwcontext.h>
}

namespace Ffmpeg {

class Packet;
class AVContextInfo;
class FormatContext;

using Metadatas = QMap<QString, QString>;

void FFMPEG_EXPORT printFfmpegInfo();

void calculatePts(const FramePtr &framePtr,
                  AVContextInfo *contextInfo,
                  FormatContext *formatContext);
void calculatePts(Packet *packet, AVContextInfo *contextInfo);

auto getCurrentHWDeviceTypes() -> QList<AVHWDeviceType>;

auto getPixelFormat(const AVCodec *codec, AVHWDeviceType type) -> AVPixelFormat;

auto compareAVRational(const AVRational &a, const AVRational &b) -> bool;

auto getMetaDatas(AVDictionary *metadata) -> Metadatas;

struct CodecInfo
{
    auto operator==(const CodecInfo &other) const -> bool { return name == other.name; }
    auto operator!=(const CodecInfo &other) const -> bool { return !(*this == other); }

    QString name;
    QString longName;
    QString displayName;
    enum AVCodecID codecId;
};

using CodecInfos = QList<CodecInfo>;

auto FFMPEG_EXPORT getCodecsInfo(AVMediaType mediaType, bool encoder) -> CodecInfos;

struct ChLayout
{
    auto operator==(const ChLayout &other) const -> bool { return channel == other.channel; }
    auto operator!=(const ChLayout &other) const -> bool { return !(*this == other); }

    AVChannel channel;
    QString channelName;
};

using ChLayouts = QList<ChLayout>;

auto FFMPEG_EXPORT getChLayouts(const QList<AVChannelLayout> &channelLayout) -> ChLayouts;

auto convertUrlToFfmpegInput(const QString &url) -> QByteArray;

} // namespace Ffmpeg

Q_DECLARE_METATYPE(Ffmpeg::CodecInfo);
