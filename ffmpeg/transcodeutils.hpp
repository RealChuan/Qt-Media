#ifndef TRANSCODEUTILS_HPP
#define TRANSCODEUTILS_HPP

#include "ffmepg_global.h"

#include <QSize>

extern "C" {
#include <libavcodec/codec_id.h>
}

namespace Ffmpeg {

namespace TranscodeUtils {

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

} // namespace TranscodeUtils

} // namespace Ffmpeg

#endif // TRANSCODEUTILS_HPP
