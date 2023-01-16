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

int compare_codec_desc(const void *a, const void *b)
{
    const AVCodecDescriptor *const *da = (const AVCodecDescriptor *const *) a;
    const AVCodecDescriptor *const *db = (const AVCodecDescriptor *const *) b;

    return (*da)->type != (*db)->type ? FFDIFFSIGN((*da)->type, (*db)->type)
                                      : strcmp((*da)->name, (*db)->name);
}

unsigned get_codecs_sorted(const AVCodecDescriptor ***rcodecs)
{
    const AVCodecDescriptor *desc = nullptr;
    const AVCodecDescriptor **codecs;
    unsigned nb_codecs = 0, i = 0;

    while ((desc = avcodec_descriptor_next(desc))) {
        nb_codecs++;
    }
    if (!(codecs = (const AVCodecDescriptor **) av_calloc(nb_codecs, sizeof(*codecs)))) {
        qCritical() << "Out of memory";
        return nb_codecs;
    }
    desc = nullptr;
    while ((desc = avcodec_descriptor_next(desc))) {
        codecs[i++] = desc;
    }
    Q_ASSERT(i == nb_codecs);
    qsort(codecs, nb_codecs, sizeof(*codecs), compare_codec_desc);
    *rcodecs = codecs;
    return nb_codecs;
}

const AVCodec *next_codec_for_id(enum AVCodecID id, void **iter, bool encoder)
{
    const AVCodec *c = nullptr;
    while ((c = av_codec_iterate(iter))) {
        if (c->id == id && (encoder ? av_codec_is_encoder(c) : av_codec_is_decoder(c))) {
            return c;
        }
    }
    return NULL;
}

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

QVector<AVCodecID> getCurrentSupportCodecIDs(AVMediaType mediaType, bool encoder)
{
    QVector<AVCodecID> codecIDs{};
    const AVCodecDescriptor **codecs{};
    auto nb_codecs = get_codecs_sorted(&codecs);

    qDebug() << av_get_media_type_string(mediaType) << "Encoders: ";
    for (unsigned i = 0; i < nb_codecs; i++) {
        const AVCodecDescriptor *desc = codecs[i];
        const AVCodec *codec = nullptr;
        void *iter = nullptr;

        if (strstr(desc->name, "_deprecated")) {
            continue;
        }

        while ((codec = next_codec_for_id(desc->id, &iter, encoder))) {
            if (desc->type != mediaType) {
                continue;
            }
            if (!codecIDs.contains(codec->id)) {
                codecIDs.append(codec->id);
            }
            auto str = QString::asprintf("%-20s %s",
                                         codec->name,
                                         codec->long_name ? codec->long_name : "");
            if (strcmp(codec->name, desc->name)) {
                str += QString::asprintf(" (codec %s)", desc->name);
            }
            qDebug() << str;
        }
    }
    av_free(codecs);
    return codecIDs;
}

} // namespace Utils

} // namespace Ffmpeg
