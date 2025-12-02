#include "mediainfo.hpp"
#include "audioframeconverter.h"

#include <utils/utils.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
}

namespace Ffmpeg {

StreamInfo::StreamInfo(struct AVStream *stream)
{
    auto *codecpar = stream->codecpar;
    auto timebase = av_q2d(stream->time_base);

    index = stream->index;
    mediaType = codecpar->codec_type;
    mediaTypeText = av_get_media_type_string(mediaType);
    startTime = QTime::fromMSecsSinceStartOfDay(stream->start_time * timebase * 1000)
                    .toString("hh:mm:ss.zzz");
    duration = QTime::fromMSecsSinceStartOfDay(stream->duration * timebase * 1000)
                   .toString("hh:mm:ss.zzz");
    codecId = codecpar->codec_id;
    codecName = avcodec_get_name(codecId);
    nbFrames = stream->nb_frames;
    bitRate = codecpar->bit_rate;
    streamSize = stream->duration * timebase * bitRate / 8;
    profile = avcodec_profile_name(codecId, codecpar->profile);
    level = codecpar->level;

    switch (mediaType) {
    case AVMEDIA_TYPE_AUDIO: {
        format = av_get_sample_fmt_name(static_cast<AVSampleFormat>(codecpar->format));
        chLayout = getAVChannelLayoutDescribe(codecpar->ch_layout);
        sampleRate = codecpar->sample_rate;
        frameSize = codecpar->frame_size;
    } break;
    case AVMEDIA_TYPE_VIDEO:
        format = av_get_pix_fmt_name(static_cast<AVPixelFormat>(codecpar->format));
        frameRate = av_q2d(stream->avg_frame_rate);
        aspectRatio = QString("%1:%2").arg(QString::number(stream->sample_aspect_ratio.num),
                                           QString::number(stream->sample_aspect_ratio.den));
        size = QSize(codecpar->width, codecpar->height);
        colorRange = av_color_range_name(codecpar->color_range);
        colorPrimaries = av_color_primaries_name(codecpar->color_primaries);
        colorTrc = av_color_transfer_name(codecpar->color_trc);
        colorSpace = av_color_space_name(codecpar->color_space);
        chromaLocation = av_chroma_location_name(codecpar->chroma_location);
        videoDelay = codecpar->video_delay;
        break;
    default: break;
    }

    metadatas = getMetaDatas(stream->metadata);

    if ((stream->disposition & AV_DISPOSITION_ATTACHED_PIC) != 0) {
        auto &pkt = stream->attached_pic;
        image = QImage::fromData(static_cast<uchar *>(pkt.data), pkt.size);
    }
}

auto StreamInfo::toJson() const -> QJsonObject
{
    QJsonObject json;
    json.insert("Index", index);
    json.insert("MediaType", mediaTypeText);
    json.insert("StartTime", startTime);
    json.insert("Duration", duration);
    json.insert("Codec", codecName);
    json.insert("NbFrames", nbFrames);
    json.insert("Format", format);
    json.insert("BitRate", Utils::formatBytes(bitRate) + "/s");
    json.insert("StreamSize", Utils::formatBytes(streamSize));
    switch (mediaType) {
    case AVMEDIA_TYPE_AUDIO:
        json.insert("ChLayout", chLayout);
        json.insert("SampleRate", sampleRate);
        json.insert("FrameSize", frameSize);
        json.insert("Profile", profile);
        break;
    case AVMEDIA_TYPE_VIDEO:
        json.insert("FrameRate", frameRate);
        json.insert("AspectRatio", aspectRatio);
        json.insert("Size",
                    QString("%1x%2").arg(QString::number(size.width()),
                                         QString::number(size.height())));
        json.insert("ColorRange", colorRange);
        json.insert("ColorPrimaries", colorPrimaries);
        json.insert("ColorTrc", colorTrc);
        json.insert("ColorSpace", colorSpace);
        json.insert("ChromaLocation", chromaLocation);
        json.insert("VideoDelay", videoDelay);
        json.insert("Profile", QString("%1(%2)").arg(profile, QString::number(level)));
    default: break;
    }

    QJsonObject metadataJson;
    for (auto iter = metadatas.constBegin(); iter != metadatas.constEnd(); ++iter) {
        metadataJson.insert(iter.key(), iter.value());
    }
    if (!metadataJson.isEmpty()) {
        json.insert("Metadata", metadataJson);
    }

    // json.insert("DefaultSelected", defaultSelected);
    // json.insert("Selected", selected);

    return json;
}

auto StreamInfo::info() const -> QString
{
    QString text = codecName;
    auto lang = metadatas.value("language");
    auto title = metadatas.value("title");
    if (!lang.isEmpty()) {
        text += "-" + lang;
    }
    if (!title.isEmpty()) {
        text += "-" + title;
    }
    if (bitRate > 0) {
        text += "-" + QString::number(bitRate);
    }
    if (!format.isEmpty()) {
        text += "-" + format;
    }

    switch (mediaType) {
    case AVMEDIA_TYPE_AUDIO:
        text += QString("-%1-%2hz").arg(chLayout, QString::number(sampleRate));
        break;
    case AVMEDIA_TYPE_VIDEO:
        text += QString("-%1fps-[%2x%3]")
                    .arg(QString::number(frameRate),
                         QString::number(size.width()),
                         QString::number(size.height()));
        break;
    default: break;
    }

    return text;
}

Chapter::Chapter(AVChapter *chapter)
{
    id = chapter->id;
    timeBase = av_q2d(chapter->time_base);
    startTime = chapter->start * timeBase;
    endTime = chapter->end * timeBase;
    startTimeText = QTime::fromMSecsSinceStartOfDay(startTime * 1000).toString("hh:mm:ss.zzz");
    endTimeText = QTime::fromMSecsSinceStartOfDay(endTime * 1000).toString("hh:mm:ss.zzz");

    metadatas = getMetaDatas(chapter->metadata);
}

auto Chapter::toJson() const -> QJsonObject
{
    QJsonObject json;
    json.insert("Id", id);
    json.insert("TimeBase", timeBase);
    json.insert("StartTime", startTimeText);
    json.insert("EndTime", endTimeText);

    QJsonObject metadataJson;
    for (auto iter = metadatas.constBegin(); iter != metadatas.constEnd(); ++iter) {
        metadataJson.insert(iter.key(), iter.value());
    }
    if (!metadataJson.isEmpty()) {
        json.insert("Metadata", metadataJson);
    }

    return json;
}

MediaInfo::MediaInfo(AVFormatContext *formatCtx)
{
    name = formatCtx->iformat->name;
    longName = formatCtx->iformat->long_name;
    url = formatCtx->url;
    startTime = formatCtx->start_time;
    startTimeText = QTime::fromMSecsSinceStartOfDay(formatCtx->start_time / 1000)
                        .toString("hh:mm:ss.zzz");
    duration = formatCtx->duration;
    durationText = QTime::fromMSecsSinceStartOfDay(formatCtx->duration / 1000)
                       .toString("hh:mm:ss.zzz");
    bitRate = formatCtx->bit_rate;
    size = duration / AV_TIME_BASE * bitRate / 8;

    metadatas = getMetaDatas(formatCtx->metadata);

    for (uint i = 0; i < formatCtx->nb_chapters; i++) {
        chapters.append(Chapter(formatCtx->chapters[i]));
    }
    for (uint i = 0; i < formatCtx->nb_streams; i++) {
        streamInfos.append(StreamInfo(formatCtx->streams[i]));
    }
}

auto MediaInfo::toJson() const -> QJsonObject
{
    QJsonObject json;
    json.insert("Name", name);
    json.insert("LongName", longName);
    json.insert("Url", url);
    json.insert("StartTime", startTimeText);
    json.insert("Duration", durationText);
    json.insert("BitRate", Utils::formatBytes(bitRate) + "/s");
    json.insert("Size", Utils::formatBytes(size));

    QJsonObject metadataJson;
    for (auto iter = metadatas.constBegin(); iter != metadatas.constEnd(); ++iter) {
        metadataJson.insert(iter.key(), iter.value());
    }
    if (!metadataJson.isEmpty()) {
        json.insert("Metadata", metadataJson);
    }

    QJsonArray streamArray;
    for (const auto &streamInfo : std::as_const(streamInfos)) {
        streamArray.append(streamInfo.toJson());
    }
    if (!streamArray.isEmpty()) {
        json.insert("StreamInfos", streamArray);
    }
    QJsonArray chapterArray;
    for (const auto &chapter : std::as_const(chapters)) {
        chapterArray.append(chapter.toJson());
    }
    if (!chapterArray.isEmpty()) {
        json.insert("Chapters", chapterArray);
    }

    return json;
}

} // namespace Ffmpeg
