#include "mediainfo.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
}

namespace Ffmpeg {

StreamInfo::StreamInfo(struct AVStream *stream)
{
    auto *codecpar = stream->codecpar;

    type = codecpar->codec_type;
    typeStr = av_get_media_type_string(type);
    startTime = QTime::fromMSecsSinceStartOfDay(stream->start_time).toString("hh:mm:ss.zzz");
    duration = QTime::fromMSecsSinceStartOfDay(stream->duration).toString("hh:mm:ss.zzz");
    codec = avcodec_get_name(codecpar->codec_id);
    nbFrames = stream->nb_frames;
    bitRate = codecpar->bit_rate;
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
        aspectRatio = av_q2d(stream->sample_aspect_ratio);
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

    AVDictionaryEntry *tag = nullptr;
    while (nullptr != (tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        QString key = tag->key;
        if (key == "language") {
            lang = QString::fromUtf8(tag->value);
        } else if (key == "title") {
            title = QString::fromUtf8(tag->value);
        }
        //qDebug() << key << ":" << QString::fromUtf8(tag->value);
    }

    if ((stream->disposition & AV_DISPOSITION_ATTACHED_PIC) != 0) {
        auto &pkt = stream->attached_pic;
        image = QImage::fromData(static_cast<uchar *>(pkt.data), pkt.size);
    }
}

auto StreamInfo::info() const -> QString
{
    QString text = codec;
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

} // namespace Ffmpeg
