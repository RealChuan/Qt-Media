#pragma once

#include <ffmpeg/ffmepg_global.h>

#include <QObject>

namespace Ffmpeg {

class FFMPEG_EXPORT Event : public QObject
{
    Q_OBJECT
public:
    enum EventType {
        None,
        MediaTracksChanged,
        DurationChanged,
        PositionChanged,
        MediaStateChanged,
        CacheSpeedChanged,
        Error,

        Pause = 100,
        Seek,
        seekRelative,
    };
    Q_ENUM(EventType);

    using QObject::QObject;

    auto operator==(const Event &other) const -> bool { return type() == other.type(); }
    auto operator!=(const Event &other) const -> bool { return !(*this == other); }

    [[nodiscard]] virtual auto type() const -> EventType { return None; }
};

using EventPtr = QSharedPointer<Event>;

} // namespace Ffmpeg
