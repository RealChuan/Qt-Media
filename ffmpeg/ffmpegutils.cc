#include "ffmpegutils.hpp"
#include "avcontextinfo.h"
#include "codeccontext.h"
#include "formatcontext.h"
#include "frame.hpp"
#include "packet.h"

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace Ffmpeg {

void calculatePts(Frame *frame, AVContextInfo *contextInfo, FormatContext *formatContext)
{
    auto tb = contextInfo->stream()->time_base;
    auto frame_rate = formatContext->guessFrameRate(contextInfo->stream());
    // 当前帧播放时长
    auto duration = (frame_rate.num && frame_rate.den
                         ? av_q2d(AVRational{frame_rate.den, frame_rate.num})
                         : 0);
    // 当前帧显示时间戳
    auto *avFrame = frame->avFrame();
    auto pts = (avFrame->pts == AV_NOPTS_VALUE) ? NAN : avFrame->pts * av_q2d(tb);
    frame->setDuration(duration * AV_TIME_BASE);
    frame->setPts(pts * AV_TIME_BASE);
    // qDebug() << "Frame duration:" << duration << "pts:" << pts << "tb:" << tb.num << tb.den
    //          << "frame_rate:" << frame_rate.num << frame_rate.den;
}

void calculatePts(Packet *packet, AVContextInfo *contextInfo)
{
    auto tb = contextInfo->stream()->time_base;
    // 当前帧播放时长
    auto *avPacket = packet->avPacket();
    auto duration = avPacket->duration * av_q2d(tb);
    // 当前帧显示时间戳
    auto pts = (avPacket->pts == AV_NOPTS_VALUE) ? NAN : avPacket->pts * av_q2d(tb);
    packet->setDuration(duration * AV_TIME_BASE);
    packet->setPts(pts * AV_TIME_BASE);
    // qDebug() << "Packet duration:" << duration << "pts:" << pts << "tb:" << tb.num << tb.den;
}

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

AVPixelFormat getPixelFormat(const AVCodec *codec, AVHWDeviceType type)
{
    auto hw_pix_fmt = AV_PIX_FMT_NONE;
    for (int i = 0;; i++) {
        auto config = avcodec_get_hw_config(codec, i);
        if (!config) {
            qWarning() << QObject::tr("Codec %1 does not support device type %2.")
                              .arg(codec->name, av_hwdevice_get_type_name(type));
            return hw_pix_fmt;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX
            && config->device_type == type) {
            qInfo() << QObject::tr("Codec %1 support device type %2.")
                           .arg(codec->name, av_hwdevice_get_type_name(type));
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }
    return hw_pix_fmt;
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
        codecs.append({codecpar->codec_type,
                       codecpar->codec_id,
                       avcodec_get_name(codecpar->codec_id),
                       {codecpar->width, codecpar->height}});
    }
    formatContextPtr->dumpFormat();
    return codecs;
}

QPair<int, int> getCodecQuantizer(const QString &codecname)
{
    QScopedPointer<AVContextInfo> contextInfoPtr(new AVContextInfo);
    if (!contextInfoPtr->initEncoder(codecname)) {
        return {-1, -1};
    }
    auto quantizer = contextInfoPtr->codecCtx()->quantizer();
    return quantizer;
}

QStringList getCurrentSupportCodecs(AVMediaType mediaType, bool encoder)
{
    QStringList codecnames{};
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
            auto name = codec->name;
            if (!codecnames.contains(name)) {
                codecnames.append(name);
            }
            auto str = QString::asprintf("%-20s %s", name, codec->long_name ? codec->long_name : "");
            if (strcmp(codec->name, desc->name)) {
                str += QString::asprintf(" (codec %s)", desc->name);
            }
            qDebug() << str;
        }
    }
    av_free(codecs);
    return codecnames;
}

} // namespace Ffmpeg
