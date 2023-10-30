#include "avcontextinfo.h"
#include "codeccontext.h"
#include "frame.hpp"
#include "packet.h"

#include <gpu/hardwaredecode.hpp>
#include <gpu/hardwareencode.hpp>

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
}

#define Error_Index -1

namespace Ffmpeg {

class AVContextInfo::AVContextInfoPrivate
{
public:
    AVContextInfoPrivate(AVContextInfo *q)
        : q_ptr(q)
    {}

    void printCodecpar()
    {
        auto codecpar = stream->codecpar;
        qInfo() << "start_time: " << stream->start_time;
        qInfo() << "duration: " << stream->duration;
        qInfo() << "nb_frames: " << stream->nb_frames;
        qInfo() << "format: " << codecpar->format;
        qInfo() << "bit_rate: " << codecpar->bit_rate;
        switch (codecpar->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
            qInfo() << "avg_frame_rate: " << av_q2d(stream->avg_frame_rate);
            qInfo() << "sample_aspect_ratio: " << av_q2d(stream->sample_aspect_ratio);
            qInfo() << "Resolution of resolution: " << codecpar->width << "x" << codecpar->height;
            qInfo() << "color_range: " << av_color_range_name(codecpar->color_range);
            qInfo() << "color_primaries: " << av_color_primaries_name(codecpar->color_primaries);
            qInfo() << "color_trc: " << av_color_transfer_name(codecpar->color_trc);
            qInfo() << "color_space: " << av_color_space_name(codecpar->color_space);
            qInfo() << "chroma_location: " << av_chroma_location_name(codecpar->chroma_location);
            qInfo() << "video_delay: " << codecpar->video_delay;
            break;
        case AVMEDIA_TYPE_AUDIO:
            qInfo() << "channels: " << codecpar->channels;
            qInfo() << "channel_layout: " << codecpar->channel_layout;
            qInfo() << "sample_rate: " << codecpar->sample_rate;
            qInfo() << "frame_size: " << codecpar->frame_size;
            break;
        default: break;
        }
    }

    void printMetaData()
    {
        QMap<QString, QString> maps;
        AVDictionaryEntry *tag = nullptr;
        while (nullptr != (tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
            maps.insert(tag->key, QString::fromUtf8(tag->value));
        }
        qDebug() << maps;
    }

    AVContextInfo *q_ptr;

    QScopedPointer<CodecContext> codecCtx; //解码器上下文
    AVStream *stream = nullptr;            //流
    int streamIndex = Error_Index;         // 索引
    QScopedPointer<HardWareDecode> hardWareDecodePtr;
    QScopedPointer<HardWareEncode> hardWareEncodePtr;
    GpuType gpuType = GpuType::NotUseGpu;
};

AVContextInfo::AVContextInfo(QObject *parent)
    : QObject(parent)
    , d_ptr(new AVContextInfoPrivate(this))
{}

AVContextInfo::~AVContextInfo() = default;

auto AVContextInfo::codecCtx() -> CodecContext *
{
    return d_ptr->codecCtx.data();
}

void AVContextInfo::resetIndex()
{
    d_ptr->streamIndex = Error_Index;
}

void AVContextInfo::setIndex(int index)
{
    d_ptr->streamIndex = index;
}

auto AVContextInfo::index() -> int
{
    return d_ptr->streamIndex;
}

auto AVContextInfo::isIndexVaild() -> bool
{
    return d_ptr->streamIndex != Error_Index;
}

void AVContextInfo::setStream(AVStream *stream)
{
    d_ptr->stream = stream;
    //    d_ptr->printCodecpar();
    //    d_ptr->printMetaData();
}

auto AVContextInfo::stream() -> AVStream *
{
    return d_ptr->stream;
}

auto AVContextInfo::initDecoder(const AVRational &frameRate) -> bool
{
    Q_ASSERT(d_ptr->stream != nullptr);
    const char *typeStr = av_get_media_type_string(d_ptr->stream->codecpar->codec_type);
    auto codec = avcodec_find_decoder(d_ptr->stream->codecpar->codec_id);
    if (!codec) {
        qWarning() << tr("%1 Codec not found.").arg(typeStr);
        return false;
    }
    d_ptr->codecCtx.reset(new CodecContext(codec));
    if (!d_ptr->codecCtx->setParameters(d_ptr->stream->codecpar)) {
        return false;
    }
    auto avCodecCtx = d_ptr->codecCtx->avCodecCtx();
    avCodecCtx->pkt_timebase = d_ptr->stream->time_base;
    d_ptr->codecCtx->setThreadCount(4);
    if (d_ptr->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        avCodecCtx->framerate = frameRate;
    }

    qInfo() << tr("Decoder name: ") << codec->name;

    return true;
}

auto AVContextInfo::initEncoder(AVCodecID codecId) -> bool
{
    auto encodec = avcodec_find_encoder(codecId);
    if (!encodec) {
        qWarning() << tr("%1 Encoder not found.").arg(avcodec_get_name(codecId));
        return false;
    }
    d_ptr->codecCtx.reset(new CodecContext(encodec));
    return true;
}

auto AVContextInfo::initEncoder(const QString &name) -> bool
{
    auto encodec = avcodec_find_encoder_by_name(name.toLocal8Bit().constData());
    if (!encodec) {
        qWarning() << tr("%1 Encoder not found.").arg(name);
        return false;
    }
    d_ptr->codecCtx.reset(new CodecContext(encodec));
    return true;
}

auto AVContextInfo::openCodec(GpuType type) -> bool
{
    d_ptr->gpuType = type;
    if (mediaType() == AVMEDIA_TYPE_VIDEO) {
        switch (d_ptr->gpuType) {
        case GpuDecode:
            d_ptr->hardWareDecodePtr.reset(new HardWareDecode);
            d_ptr->hardWareDecodePtr->initPixelFormat(d_ptr->codecCtx->codec());
            d_ptr->hardWareDecodePtr->initHardWareDevice(d_ptr->codecCtx.data());
            break;
        case GpuEncode:
            d_ptr->hardWareEncodePtr.reset(new HardWareEncode);
            d_ptr->hardWareEncodePtr->initEncoder(d_ptr->codecCtx->codec());
            d_ptr->hardWareEncodePtr->initHardWareDevice(d_ptr->codecCtx.data());
            break;
        default: break;
        }
    }

    //用于初始化pCodecCtx结构
    if (!d_ptr->codecCtx->open()) {
        return false;
    }
    return true;
}

auto AVContextInfo::decodeSubtitle2(const QSharedPointer<Subtitle> &subtitlePtr,
                                    const QSharedPointer<Packet> &packetPtr) -> bool
{
    return d_ptr->codecCtx->decodeSubtitle2(subtitlePtr.data(), packetPtr.data());
}

auto AVContextInfo::decodeFrame(const QSharedPointer<Packet> &packetPtr)
    -> std::vector<QSharedPointer<Frame>>
{
    std::vector<FramePtr> framePtrs;
    if (!d_ptr->codecCtx->sendPacket(packetPtr.data())) {
        return framePtrs;
    }
    FramePtr framePtr(new Frame);
    while (d_ptr->codecCtx->receiveFrame(framePtr.data())) {
        if (d_ptr->gpuType == GpuDecode && mediaType() == AVMEDIA_TYPE_VIDEO) {
            bool ok = false;
            framePtr = d_ptr->hardWareDecodePtr->transFromGpu(framePtr, ok);
            if (!ok) {
                return framePtrs;
            }
        }
        framePtrs.push_back(framePtr);
        framePtr.reset(new Frame);
    }
    return framePtrs;
}

auto AVContextInfo::encodeFrame(const QSharedPointer<Frame> &framePtr)
    -> std::vector<QSharedPointer<Packet>>
{
    std::vector<PacketPtr> packetPtrs{};
    auto frame_tmp_ptr = framePtr;
    if (d_ptr->gpuType == GpuEncode && mediaType() == AVMEDIA_TYPE_VIDEO
        && framePtr->avFrame() != nullptr) {
        bool ok = false;
        frame_tmp_ptr = d_ptr->hardWareEncodePtr->transToGpu(d_ptr->codecCtx.data(), framePtr, ok);
        if (!ok) {
            return packetPtrs;
        }
    }
    if (!d_ptr->codecCtx->sendFrame(frame_tmp_ptr.data())) {
        return packetPtrs;
    }
    PacketPtr packetPtr(new Packet);
    while (d_ptr->codecCtx->receivePacket(packetPtr.get())) {
        packetPtrs.push_back(packetPtr);
        packetPtr.reset(new Packet);
    }
    return packetPtrs;
}

auto AVContextInfo::calTimebase() const -> double
{
    return av_q2d(d_ptr->stream->time_base);
}

auto AVContextInfo::timebase() const -> AVRational
{
    return d_ptr->stream->time_base;
}

auto AVContextInfo::fps() const -> double
{
    return av_q2d(d_ptr->stream->avg_frame_rate);
}

auto AVContextInfo::fames() const -> qint64
{
    return d_ptr->stream->nb_frames;
}

auto AVContextInfo::resolutionRatio() const -> QSize
{
    return {d_ptr->stream->codecpar->width, d_ptr->stream->codecpar->height};
}

auto AVContextInfo::mediaType() const -> AVMediaType
{
    return d_ptr->stream->codecpar->codec_type;
}

auto AVContextInfo::mediaTypeString() const -> QString
{
    return av_get_media_type_string(mediaType());
}

auto AVContextInfo::isDecoder() const -> bool
{
    return d_ptr->codecCtx->isDecoder();
}

auto AVContextInfo::gpuType() const -> AVContextInfo::GpuType
{
    return d_ptr->gpuType;
}

auto AVContextInfo::pixfmt() const -> AVPixelFormat
{
    if (d_ptr->gpuType == GpuEncode && mediaType() == AVMEDIA_TYPE_VIDEO
        && d_ptr->hardWareEncodePtr->isVaild()) {
        return d_ptr->hardWareEncodePtr->swFormat();
    }
    return d_ptr->codecCtx->avCodecCtx()->pix_fmt;
}

} // namespace Ffmpeg
