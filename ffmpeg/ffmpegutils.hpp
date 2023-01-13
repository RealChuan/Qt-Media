#ifndef FFMPEGUTILS_HPP
#define FFMPEGUTILS_HPP

#include "ffmepg_global.h"

#include <QSize>

extern "C" {
#include <libavcodec/codec_id.h>
#include <libavutil/hwcontext.h>
}

namespace Ffmpeg {

namespace Utils {

void FFMPEG_EXPORT printFfmpegInfo();

QVector<AVHWDeviceType> getCurrentHWDeviceTypes();

struct CodecInfo
{
    AVMediaType mediaType = AVMEDIA_TYPE_UNKNOWN;
    AVCodecID codecID = AV_CODEC_ID_NONE;
    QSize size = QSize(-1, -1);
};

QVector<CodecInfo> FFMPEG_EXPORT getFileCodecInfo(const QString &filePath);

QPair<int, int> FFMPEG_EXPORT getCodecQuantizer(AVCodecID codecId);

bool FFMPEG_EXPORT isSupportVideoEncoder(AVCodecID codecId);

bool FFMPEG_EXPORT isSupportAudioEncoder(AVCodecID codecId);

} // namespace Utils

} // namespace Ffmpeg

#endif // FFMPEGUTILS_HPP
