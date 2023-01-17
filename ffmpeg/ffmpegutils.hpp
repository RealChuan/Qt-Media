#ifndef FFMPEGUTILS_HPP
#define FFMPEGUTILS_HPP

#include "ffmepg_global.h"

#include <QSize>

extern "C" {
#include <libavcodec/codec_id.h>
#include <libavutil/hwcontext.h>
}

struct AVCodec;

namespace Ffmpeg {

namespace Utils {

void FFMPEG_EXPORT printFfmpegInfo();

QVector<AVHWDeviceType> getCurrentHWDeviceTypes();

AVPixelFormat getPixelFormat(const AVCodec *codec, AVHWDeviceType type);

struct CodecInfo
{
    AVMediaType mediaType = AVMEDIA_TYPE_UNKNOWN;
    AVCodecID id = AV_CODEC_ID_NONE;
    QString name;
    QSize size = QSize(-1, -1);
};

QVector<CodecInfo> FFMPEG_EXPORT getFileCodecInfo(const QString &filePath);

QPair<int, int> FFMPEG_EXPORT getCodecQuantizer(const QString &codecname);

QStringList FFMPEG_EXPORT getCurrentSupportCodecs(AVMediaType mediaType, bool encoder);

} // namespace Utils

} // namespace Ffmpeg

#endif // FFMPEGUTILS_HPP
