#ifndef MEDIAINFO_HPP
#define MEDIAINFO_HPP

#include "mpv_global.h"

#include <QtCore>

namespace Mpv {

struct MPV_LIB_EXPORT TraskInfo
{
    TraskInfo() = default;
    explicit TraskInfo(const QJsonObject &obj);

    [[nodiscard]] auto text() const -> QString;

    bool albumart = false;
    QString codec;
    bool isDefault = false;
    bool dependent = false;
    bool external = false;
    int ff_index = 0;
    bool forced = false;
    bool hearing_impaired = false;
    QVariant id;
    bool image = false;
    QString lang;
    bool selected = false;
    int src_id = 0;
    QString title;
    QString type;
    bool visual_impaired = false;

    QSize size = QSize(0, 0);
    double fps = 0.0;

    int channelCount = 0;
    int samplerate = 0;
};

using TraskInfos = QList<TraskInfo>;

struct MPV_LIB_EXPORT Chapter
{
    Chapter() = default;
    explicit Chapter(const QJsonObject &obj);

    QString title;
    qint64 milliseconds;
};

using Chapters = QList<Chapter>;

} // namespace Mpv

Q_DECLARE_METATYPE(Mpv::TraskInfo)

#endif // MEDIAINFO_HPP
