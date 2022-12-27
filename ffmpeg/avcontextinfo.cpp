#include "avcontextinfo.h"
#include "codeccontext.h"
#include "frame.hpp"
#include "hardwaredecode.hpp"

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
}

#define Error_Index -1

namespace Ffmpeg {

struct AVContextInfo::AVContextInfoPrivate
{
    QScopedPointer<CodecContext> codecCtx; //解码器上下文
    AVStream *stream = nullptr;            //流
    int streamIndex = Error_Index;         // 索引
    QScopedPointer<HardWareDecode> hardWareDecode;
    bool gpuDecode = false;
};

AVContextInfo::AVContextInfo(QObject *parent)
    : QObject(parent)
    , d_ptr(new AVContextInfoPrivate)
{}

AVContextInfo::~AVContextInfo() {}

void AVContextInfo::copyToCodecParameters(AVContextInfo *dst)
{
    d_ptr->codecCtx->copyToCodecParameters(dst->d_ptr->codecCtx.data());
}

CodecContext *AVContextInfo::codecCtx()
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

int AVContextInfo::index()
{
    return d_ptr->streamIndex;
}

bool AVContextInfo::isIndexVaild()
{
    return d_ptr->streamIndex != Error_Index;
}

void AVContextInfo::setStream(AVStream *stream)
{
    d_ptr->stream = stream;
    showCodecpar();
    showMetaData();
}

AVStream *AVContextInfo::stream()
{
    return d_ptr->stream;
}

bool AVContextInfo::initDecoder(const AVRational &frameRate)
{
    Q_ASSERT(d_ptr->stream != nullptr);
    const char *typeStr = av_get_media_type_string(d_ptr->stream->codecpar->codec_type);
    AVCodec *codec = avcodec_find_decoder(d_ptr->stream->codecpar->codec_id);
    if (!codec) {
        qWarning() << tr("%1 Codec not found.").arg(typeStr);
        return false;
    }
    d_ptr->codecCtx.reset(new CodecContext(codec));
    if (!d_ptr->codecCtx->setParameters(d_ptr->stream->codecpar)) {
        return false;
    }
    d_ptr->codecCtx->setTimebase(d_ptr->stream->time_base);
    d_ptr->codecCtx->setThreadCount(4);
    if (d_ptr->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        d_ptr->codecCtx->setFrameRate(frameRate);
    }
    connect(d_ptr->codecCtx.data(),
            &CodecContext::error,
            this,
            &AVContextInfo::error,
            Qt::UniqueConnection);

    qInfo() << tr("Decoder name: ") << codec->name;

    return true;
}

bool AVContextInfo::initEncoder(AVCodecID codecId)
{
    auto encodec = avcodec_find_encoder(codecId);
    if (!encodec) {
        qWarning() << tr("%1 Encoder not found.").arg(avcodec_get_name(codecId));
        return false;
    }
    d_ptr->codecCtx.reset(new CodecContext(encodec));
    return true;
}

bool AVContextInfo::initEncoder(const QString &name)
{
    auto encodec = avcodec_find_encoder_by_name(name.toLocal8Bit().constData());
    if (!encodec) {
        qWarning() << tr("%1 Encoder not found.").arg(name);
        return false;
    }
    d_ptr->codecCtx.reset(new CodecContext(encodec));
    return true;
}

bool AVContextInfo::openCodec(bool useGpu)
{
    d_ptr->gpuDecode = useGpu;
    if (mediaType() == AVMEDIA_TYPE_VIDEO && d_ptr->gpuDecode) {
        d_ptr->hardWareDecode.reset(new HardWareDecode);
        d_ptr->hardWareDecode->initPixelFormat(d_ptr->codecCtx->codec());
        d_ptr->hardWareDecode->initHardWareDevice(d_ptr->codecCtx.data());
    }

    //用于初始化pCodecCtx结构
    if (!d_ptr->codecCtx->open()) {
        return false;
    }
    return true;
}

bool AVContextInfo::sendPacket(Packet *packet)
{
    return d_ptr->codecCtx->sendPacket(packet);
}

bool AVContextInfo::receiveFrame(Frame *frame)
{
    return d_ptr->codecCtx->receiveFrame(frame);
}

bool AVContextInfo::decodeSubtitle2(Subtitle *subtitle, Packet *packet)
{
    return d_ptr->codecCtx->decodeSubtitle2(subtitle, packet);
}

Frame *AVContextInfo::decodeFrame(Packet *packet)
{
    if (!sendPacket(packet)) {
        return nullptr;
    }
    std::unique_ptr<Frame> framePtr(new Frame);
    if (!receiveFrame(framePtr.get())) { // 一个packet一个视频帧
        return nullptr;
    }
    if (d_ptr->gpuDecode && mediaType() == AVMEDIA_TYPE_VIDEO) {
        bool ok = false;
        framePtr.reset(d_ptr->hardWareDecode->transforFrame(framePtr.get(), ok));
        if (!ok) {
            return nullptr;
        }
    }
    return framePtr.release();
}

void AVContextInfo::flush()
{
    d_ptr->codecCtx->flush();
}

AVRational AVContextInfo::timebase() const
{
    return d_ptr->stream->time_base;
    //return av_q2d(d_ptr->stream->time_base);
}

double AVContextInfo::fps() const
{
    return av_q2d(d_ptr->stream->avg_frame_rate);
}

qint64 AVContextInfo::fames() const
{
    return d_ptr->stream->nb_frames;
}

QSize AVContextInfo::resolutionRatio() const
{
    return {d_ptr->stream->codecpar->width, d_ptr->stream->codecpar->height};
}

AVMediaType AVContextInfo::mediaType() const
{
    return d_ptr->stream->codecpar->codec_type;
}

QString AVContextInfo::mediaTypeString() const
{
    return av_get_media_type_string(mediaType());
}

bool AVContextInfo::isDecoder() const
{
    return d_ptr->codecCtx->isDecoder();
}

bool AVContextInfo::isGpuDecode()
{
    return d_ptr->gpuDecode;
}

void AVContextInfo::showCodecpar()
{
    auto codecpar = d_ptr->stream->codecpar;
    qInfo() << "start_time: " << d_ptr->stream->start_time;
    qInfo() << "duration: " << d_ptr->stream->duration;
    qInfo() << "nb_frames: " << d_ptr->stream->nb_frames;
    qInfo() << "index_entries_allocated_size: " << d_ptr->stream->index_entries_allocated_size;
    qInfo() << "format: " << codecpar->format;
    qInfo() << "bit_rate: " << codecpar->bit_rate;
    switch (mediaType()) {
    case AVMEDIA_TYPE_VIDEO:
        qInfo() << "avg_frame_rate: " << av_q2d(d_ptr->stream->avg_frame_rate);
        qInfo() << "sample_aspect_ratio: " << av_q2d(d_ptr->stream->sample_aspect_ratio);
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

void AVContextInfo::showMetaData()
{
    QMap<QString, QString> maps;
    AVDictionaryEntry *tag = nullptr;
    while (nullptr != (tag = av_dict_get(d_ptr->stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        maps.insert(tag->key, QString::fromUtf8(tag->value));
    }
    qDebug() << maps;
}

} // namespace Ffmpeg
