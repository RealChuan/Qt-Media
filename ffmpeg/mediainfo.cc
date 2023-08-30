#include "mediainfo.hpp"

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

void StreamInfo::parseStreamData(struct AVStream *stream)
{
    auto *codecpar = stream->codecpar;
    type = codecpar->codec_type;
    codec = avcodec_get_name(codecpar->codec_id);
    bitRate = codecpar->bit_rate;

    fps = av_q2d(stream->avg_frame_rate);
    width = codecpar->width;
    height = codecpar->height;

    channels = codecpar->ch_layout.nb_channels;
    sampleRate = codecpar->sample_rate;

    AVDictionaryEntry *tag = nullptr;
    while (nullptr != (tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        QString key = tag->key;
        if (key == "language") {
            lang = QString::fromUtf8(tag->value);
        } else if (key == "title") {
            title = QString::fromUtf8(tag->value);
        } else if (key == "NUMBER_OF_BYTES") {
            NUMBER_OF_BYTES = QString::fromUtf8(tag->value).toLongLong();
        }
        //qDebug() << key << ":" << QString::fromUtf8(tag->value);
    }

    if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
        auto &pkt = stream->attached_pic;
        image = QImage::fromData((uchar *) pkt.data, pkt.size);
    }
}

QString StreamInfo::info() const
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

    switch (type) {
    case AVMEDIA_TYPE_AUDIO:
        text += "-" + QString::number(channels) + "-" + QString::number(sampleRate);
        break;
    case AVMEDIA_TYPE_VIDEO:
        text += "-" + QString::number(fps) + "-" + QString::number(width) + "-"
                + QString::number(height);
        break;
    default: break;
    }

    return text;
}

} // namespace Ffmpeg
