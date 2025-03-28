#include "ffmpegutils.hpp"
#include "audioframeconverter.h"
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
    auto timeBase = av_q2d(contextInfo->timebase());
    auto frameRate = formatContext->guessFrameRate(contextInfo->stream());
    // 当前帧播放时长
    auto duration = ((frameRate.num != 0) && (frameRate.den != 0)
                         ? av_q2d(AVRational{frameRate.den, frameRate.num})
                         : 0);
    // 当前帧显示时间戳
    auto *avFrame = frame->avFrame();
    auto pts = (avFrame->pts == AV_NOPTS_VALUE) ? NAN : avFrame->pts * timeBase;
    frame->setDuration(duration * AV_TIME_BASE);
    frame->setPts(pts * AV_TIME_BASE);
    // qDebug() << "Frame duration:" << duration << "pts:" << pts << "tb:" << tb.num << tb.den
    //          << "frame_rate:" << frame_rate.num << frame_rate.den;
}

void calculatePts(Packet *packet, AVContextInfo *contextInfo)
{
    auto timeBase = av_q2d(contextInfo->timebase());
    // 当前帧播放时长
    auto *avPacket = packet->avPacket();
    auto duration = avPacket->duration * timeBase;
    // 当前帧显示时间戳
    auto pts = (avPacket->pts == AV_NOPTS_VALUE) ? NAN : avPacket->pts * timeBase;
    packet->setDuration(duration * AV_TIME_BASE);
    packet->setPts(pts * AV_TIME_BASE);
    // qDebug() << "Packet duration:" << duration << "pts:" << pts << "tb:" << tb.num << tb.den;
}

auto compare_codec_desc(const void *a, const void *b) -> int
{
    const auto *da = static_cast<const AVCodecDescriptor *const *>(a);
    const auto *db = static_cast<const AVCodecDescriptor *const *>(b);

    return (*da)->type != (*db)->type ? FFDIFFSIGN((*da)->type, (*db)->type)
                                      : strcmp((*da)->name, (*db)->name);
}

auto get_codecs_sorted(const AVCodecDescriptor ***rcodecs) -> unsigned
{
    const AVCodecDescriptor *desc = nullptr;
    const AVCodecDescriptor **codecs;
    unsigned nb_codecs = 0;
    unsigned i = 0;

    while ((desc = avcodec_descriptor_next(desc)) != nullptr) {
        nb_codecs++;
    }
    if ((codecs = static_cast<const AVCodecDescriptor **>(av_calloc(nb_codecs, sizeof(*codecs))))
        == nullptr) {
        qCritical() << "Out of memory";
        return nb_codecs;
    }
    desc = nullptr;
    while ((desc = avcodec_descriptor_next(desc)) != nullptr) {
        codecs[i++] = desc;
    }
    Q_ASSERT(i == nb_codecs);
    qsort(codecs, nb_codecs, sizeof(*codecs), compare_codec_desc);
    *rcodecs = codecs;
    return nb_codecs;
}

auto next_codec_for_id(enum AVCodecID id, void **iter, bool encoder) -> const AVCodec *
{
    const AVCodec *c = nullptr;
    while ((c = av_codec_iterate(iter)) != nullptr) {
        if (c->id == id && ((encoder ? av_codec_is_encoder(c) : av_codec_is_decoder(c)) != 0)) {
            return c;
        }
    }
    return nullptr;
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

auto getCurrentHWDeviceTypes() -> QList<AVHWDeviceType>
{
    static QList<AVHWDeviceType> types{};
    if (types.isEmpty()) {
        auto type = AV_HWDEVICE_TYPE_NONE; // ffmpeg支持的硬件解码器
        QStringList list;
        while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
            types.append(type);
            const auto *ctype = av_hwdevice_get_type_name(type); // 获取AVHWDeviceType的字符串名称。
            if (ctype != nullptr) {
                list.append(QString(ctype));
            }
        }
        qInfo() << QObject::tr("Current hardware decoders: ") << list;
    }
    return types;
}

auto getPixelFormat(const AVCodec *codec, AVHWDeviceType type) -> AVPixelFormat
{
    auto hw_pix_fmt = AV_PIX_FMT_NONE;
    for (int i = 0;; i++) {
        const auto *config = avcodec_get_hw_config(codec, i);
        if (config == nullptr) {
            qWarning() << QObject::tr("Codec %1 does not support device type %2.")
                              .arg(codec->name, av_hwdevice_get_type_name(type));
            return hw_pix_fmt;
        }
        if (((config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) != 0)
            && config->device_type == type) {
            qInfo() << QObject::tr("Codec %1 support device type %2.")
                           .arg(codec->name, av_hwdevice_get_type_name(type));
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }
    return hw_pix_fmt;
}

auto compareAVRational(const AVRational &a, const AVRational &b) -> bool
{
    return a.den == b.den && a.num == b.num;
}

auto getCodecsInfo(AVMediaType mediaType, bool encoder) -> CodecInfos
{
    CodecInfos codecInfos;
    const AVCodecDescriptor **codecs{};
    auto nb_codecs = get_codecs_sorted(&codecs);

    qDebug() << av_get_media_type_string(mediaType) << "Encoders: ";
    for (unsigned i = 0; i < nb_codecs; i++) {
        const AVCodecDescriptor *desc = codecs[i];
        const AVCodec *codec = nullptr;
        void *iter = nullptr;

        if (strstr(desc->name, "_deprecated") != nullptr) {
            continue;
        }

        while ((codec = next_codec_for_id(desc->id, &iter, encoder)) != nullptr) {
            if (desc->type != mediaType) {
                continue;
            }
            const auto *name = codec->name;
            CodecInfo codecInfo;
            codecInfo.name = QString::fromUtf8(name);
            codecInfo.longName = QString::fromUtf8(desc->long_name);
            codecInfo.displayName = QString("%1 (%2)").arg(codecInfo.longName, codecInfo.name);
            codecInfo.codecId = codec->id;
            codecInfos.append(codecInfo);
            auto str = QString::asprintf("%-20s %s",
                                         name,
                                         codec->long_name != nullptr ? codec->long_name : "");
            if (strcmp(codec->name, desc->name) != 0) {
                str += QString::asprintf(" (codec %s)", desc->name);
            }
            qDebug() << str;
        }
    }
    av_free(codecs);
    return codecInfos;
}

auto getMetaDatas(AVDictionary *metadata) -> Metadatas
{
    Metadatas metadatas{};
    AVDictionaryEntry *tag = nullptr;
    while (nullptr != (tag = av_dict_get(metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        metadatas.insert(QString::fromUtf8(tag->key), QString::fromUtf8(tag->value));
    }
    return metadatas;
}

auto getChLayouts(const QList<AVChannelLayout> &channelLayout) -> ChLayouts
{
    static ChLayouts s_chLayouts;
    if (s_chLayouts.isEmpty()) {
        QList<qint64> channels{AV_CH_LAYOUT_MONO,
                               AV_CH_LAYOUT_STEREO,
                               AV_CH_LAYOUT_2_1,
                               AV_CH_LAYOUT_QUAD,
                               AV_CH_LAYOUT_5POINT0_BACK,
                               AV_CH_LAYOUT_6POINT0_FRONT,
                               AV_CH_LAYOUT_6POINT1,
                               AV_CH_LAYOUT_7POINT1};

        for (const auto &channel : std::as_const(channels)) {
            auto ch = static_cast<AVChannel>(channel);
            // char name[64] = {0};
            // char description[128] = {0};
            // av_channel_name(name, sizeof(name), ch);
            // av_channel_description(description, sizeof(description), ch);
            // auto text = QString("%1 (%2)").arg(name).arg(description);
            AVChannelLayout chLayout = {AV_CHANNEL_ORDER_UNSPEC};
            av_channel_layout_from_mask(&chLayout, channel);
            s_chLayouts.append({ch, getAVChannelLayoutDescribe(chLayout)});
        }
    }

    ChLayouts chLayouts;
    for (const auto &ch : std::as_const(channelLayout)) {
        chLayouts.append({static_cast<AVChannel>(ch.u.mask), getAVChannelLayoutDescribe(ch)});
    }
    return chLayouts.isEmpty() ? s_chLayouts : chLayouts;
}

QByteArray convertUrlToFfmpegInput(const QString &url)
{
    QByteArray inpuUrl = QUrl::fromUserInput(url).isLocalFile() ? url.toUtf8()
                                                                : QUrl(url).toEncoded();
    return inpuUrl;
}

} // namespace Ffmpeg
