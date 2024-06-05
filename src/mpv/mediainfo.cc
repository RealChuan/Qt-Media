#include "mediainfo.hpp"

namespace Mpv {

TraskInfo::TraskInfo(const QJsonObject &obj)
{
    albumart = obj.value("albumart").toBool();
    codec = obj.value("codec").toString();
    isDefault = obj.value("default").toBool();
    dependent = obj.value("dependent").toBool();
    external = obj.value("external").toBool();
    ff_index = obj.value("ff-index").toInt();
    forced = obj.value("forced").toBool();
    hearing_impaired = obj.value("hearing-impaired").toBool();
    id = obj.value("id").toInt();
    image = obj.value("image").toBool();
    lang = obj.value("lang").toString();
    selected = obj.value("selected").toBool();
    src_id = obj.value("src-id").toInt();
    title = obj.value("title").toString();
    type = obj.value("type").toString();
    visual_impaired = obj.value("visual-impaired").toBool();

    size = QSize(obj.value("demux-w").toInt(), obj.value("demux-h").toInt());
    fps = obj.value("demux-fps").toDouble();

    channelCount = obj.value("demux-channel-count").toInt();
    samplerate = obj.value("demux-samplerate").toInt();
}

auto TraskInfo::text() const -> QString
{
    auto text = title;
    if (text.isEmpty()) {
        text = lang;
    } else if (!lang.isEmpty()) {
        text += "-" + lang;
    }

    if (text.isEmpty()) {
        text = codec;
    } else if (!codec.isEmpty()) {
        text += "-" + codec;
    }
    if (text.isEmpty()) {
        return type;
    }

    if (isDefault) {
        text += "-Default";
    }
    if (type == "video") {
        text += QString("-%1fps-[%2x%3]")
                    .arg(QString::number(fps, 'f', 2),
                         QString::number(size.width()),
                         QString::number(size.height()));
    } else if (type == "audio") {
        text += QString("-%1-%2hz").arg(QString::number(channelCount), QString::number(samplerate));
    }

    return text;
}

Chapter::Chapter(const QJsonObject &obj)
{
    title = obj.value("title").toString();
    milliseconds = obj.value("time").toDouble() * 1000;
}

} // namespace Mpv
