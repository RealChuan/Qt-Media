#pragma once

#include <ffmpeg/ffmepg_global.h>

#include <QObject>

namespace Ffmpeg {

class FFMPEG_EXPORT PropertyChangeEvent : public QObject
{
    Q_OBJECT
public:
    enum EventType {
        None,
        Duration,
        Position,
        MediaTrack,
        MediaState,
        CacheSpeed,
        SeekChanged,
        Error
    };
    Q_ENUM(EventType);

    using QObject::QObject;

    auto operator==(const PropertyChangeEvent &other) const -> bool
    {
        return type() == other.type();
    }
    auto operator!=(const PropertyChangeEvent &other) const -> bool { return !(*this == other); }

    [[nodiscard]] virtual auto type() const -> EventType { return None; }
};

using PropertyChangeEventPtr = QSharedPointer<PropertyChangeEvent>;

class FFMPEG_EXPORT Event : public QObject
{
    Q_OBJECT
public:
    enum EventType {
        None,
        OpenMedia,
        CloseMedia,
        AudioTarck,
        VideoTrack,
        SubtitleTrack,
        Volume,
        Speed,
        Gpu,

        Pause = 100,
        Seek,
        SeekRelative
    };
    Q_ENUM(EventType);

    using QObject::QObject;

    auto operator==(const Event &other) const -> bool { return type() == other.type(); }
    auto operator!=(const Event &other) const -> bool { return !(*this == other); }

    [[nodiscard]] virtual auto type() const -> EventType { return None; }
};

using EventPtr = QSharedPointer<Event>;

} // namespace Ffmpeg
