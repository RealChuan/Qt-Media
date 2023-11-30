#include "mediainfo.hpp"

#include <utils/utils.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
}

namespace Ffmpeg {

StreamInfo::StreamInfo(struct AVStream *stream)
{
    auto *codecpar = stream->codecpar;
    auto timebase = av_q2d(stream->time_base);

    type = codecpar->codec_type;
    typeStr = av_get_media_type_string(type);
    startTime = QTime::fromMSecsSinceStartOfDay(stream->start_time * timebase * 1000)
                    .toString("hh:mm:ss.zzz");
    duration = QTime::fromMSecsSinceStartOfDay(stream->duration * timebase * 1000)
                   .toString("hh:mm:ss.zzz");
    codec = avcodec_get_name(codecpar->codec_id);
    nbFrames = stream->nb_frames;
    bitRate = codecpar->bit_rate;
    streamSize = stream->duration * timebase * bitRate / 8;
    profile = avcodec_profile_name(codecpar->codec_id, codecpar->profile);
    level = codecpar->level;

    switch (type) {
    case AVMEDIA_TYPE_AUDIO: {
        format = av_get_sample_fmt_name(static_cast<AVSampleFormat>(codecpar->format));
        char buf[64] = {0};
        av_channel_layout_describe(&codecpar->ch_layout, buf, sizeof(buf));
        chLayout = QString::fromUtf8(buf);
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
    json.insert("MediaType", typeStr);
    json.insert("StartTime", startTime);
    json.insert("Duration", duration);
    json.insert("Codec", codec);
    json.insert("NbFrames", nbFrames);
    json.insert("Format", format);
    json.insert("BitRate", Utils::convertBytesToString(bitRate) + "/s");
    json.insert("StreamSize", Utils::convertBytesToString(streamSize));
    switch (type) {
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
    QString text = codec;
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
    text += "-" + format;

    switch (type) {
    case AVMEDIA_TYPE_AUDIO: text += "-" + chLayout + "-" + QString::number(sampleRate); break;
    case AVMEDIA_TYPE_VIDEO:
        text += "-" + QString::number(frameRate) + "-" + QString::number(size.width()) + "x"
                + QString::number(size.height());
        break;
    default: break;
    }

    return text;
}

Chapter::Chapter(AVChapter *chapter)
{
    id = chapter->id;
    timeBase = av_q2d(chapter->time_base);
    startTime = QTime::fromMSecsSinceStartOfDay(chapter->start * timeBase).toString("hh:mm:ss.zzz");
    endTime = QTime::fromMSecsSinceStartOfDay(chapter->end * timeBase).toString("hh:mm:ss.zzz");

    metadatas = getMetaDatas(chapter->metadata);
}

auto Chapter::toJson() const -> QJsonObject
{
    QJsonObject json;
    json.insert("Id", id);
    json.insert("TimeBase", timeBase);
    json.insert("StartTime", startTime);
    json.insert("EndTime", endTime);

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
    startTime = QTime::fromMSecsSinceStartOfDay(formatCtx->start_time / 1000)
                    .toString("hh:mm:ss.zzz");
    duration = QTime::fromMSecsSinceStartOfDay(formatCtx->duration / 1000).toString("hh:mm:ss.zzz");
    bitRate = formatCtx->bit_rate;
    size = formatCtx->duration / AV_TIME_BASE * bitRate / 8;

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
    json.insert("StartTime", startTime);
    json.insert("Duration", duration);
    json.insert("BitRate", Utils::convertBytesToString(bitRate) + "/s");
    json.insert("Size", Utils::convertBytesToString(size));

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
