#include "ffmpegutils.hpp"
#include "avcontextinfo.h"
#include "codeccontext.h"
#include "formatcontext.h"

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace Ffmpeg {

namespace Utils {

void printFfmpegInfo()
{
    qInfo() << "ffmpeg version:" << av_version_info();
    qInfo() << "libavfilter version:" << avfilter_version();
    qInfo() << "libavformat version:" << avformat_version();
    qInfo() << "libavcodec version:" << avcodec_version();
    qInfo() << "libswresample version:" << swresample_version();
    qInfo() << "libswscale version:" << swscale_version();
    qInfo() << "libavutil version:" << avutil_version();
    qInfo() << avutil_license();
    qInfo() << avutil_configuration();
}

QVector<AVHWDeviceType> getCurrentHWDeviceTypes()
{
    static QVector<AVHWDeviceType> types{};
    if (types.isEmpty()) {
        auto type = AV_HWDEVICE_TYPE_NONE; // ffmpeg支持的硬件解码器
        QStringList list;
        while ((type = av_hwdevice_iterate_types(type))
               != AV_HWDEVICE_TYPE_NONE) // 遍历支持的设备类型。
        {
            types.append(type);
            auto ctype = av_hwdevice_get_type_name(type); // 获取AVHWDeviceType的字符串名称。
            if (ctype) {
                list.append(QString(ctype));
            }
        }
        qInfo() << QObject::tr("Current hardware decoders: ") << list;
    }
    return types;
}

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

} // namespace Utils

} // namespace Ffmpeg
