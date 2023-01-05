#include "transcodeutils.hpp"
#include "avcontextinfo.h"
#include "codeccontext.h"
#include "formatcontext.h"

extern "C" {
#include "libavformat/avformat.h"
}

namespace Ffmpeg {

namespace TranscodeUtils {

QVector<CodecInfo> getFileCodecInfo(const QString &filePath)
{
    QVector<CodecInfo> codecs{};
    QScopedPointer<FormatContext> formatContextPtr(new FormatContext);
    auto ret = formatContextPtr->openFilePath(filePath);
    if (!ret) {
        return codecs;
    }
    formatContextPtr->findStream();
    auto stream_num = formatContextPtr->streams();
    for (int i = 0; i < stream_num; i++) {
        auto codecpar = formatContextPtr->stream(i)->codecpar;
        codecs.append(
            {codecpar->codec_type, codecpar->codec_id, {codecpar->width, codecpar->height}});
    }
    formatContextPtr->dumpFormat();
    return codecs;
}

QPair<int, int> getCodecQuantizer(AVCodecID codecId)
{
    QScopedPointer<AVContextInfo> contextInfoPtr(new AVContextInfo);
    if (!contextInfoPtr->initEncoder(codecId)) {
        return {-1, -1};
    }
    auto quantizer = contextInfoPtr->codecCtx()->quantizer();
    return quantizer;
}

bool isSupportVideoEncoder(AVCodecID codecId)
{
    QScopedPointer<AVContextInfo> contextInfo(new AVContextInfo);
    if (!contextInfo->initEncoder(codecId)) {
        return false;
    }
    auto pixfmts = contextInfo->codecCtx()->supportPixFmts();
    return !pixfmts.isEmpty();
}

bool isSupportAudioEncoder(AVCodecID codecId)
{
    QScopedPointer<AVContextInfo> contextInfo(new AVContextInfo);
    if (!contextInfo->initEncoder(codecId)) {
        return false;
    }
    auto samplefmts = contextInfo->codecCtx()->supportSampleFmts();
    return !samplefmts.isEmpty();
}

} // namespace TranscodeUtils

} // namespace Ffmpeg
