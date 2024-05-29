#pragma once

#include <ffmpeg/ffmepg_global.h>

#include <QObject>

namespace Ffmpeg {

class FFMPEG_EXPORT PropertyChangeEvent : public QObject
{
    Q_OBJECT
public:
    enum class EventType {
        None,
        Duration,
        Position,
        MediaTrack,
        MediaState,
        CacheSpeed,
        SeekChanged,
        PreviewFramesChanged,
        AVError,
        Error
    };
    Q_ENUM(EventType);

    using QObject::QObject;

    auto operator==(const PropertyChangeEvent &other) const -> bool
    {
        return type() == other.type();
    }
    auto operator!=(const PropertyChangeEvent &other) const -> bool { return !(*this == other); }

    void setType(EventType type) { m_type = type; }
    [[nodiscard]] virtual auto type() const -> EventType { return m_type; }

private:
    EventType m_type = EventType::None;
};

using PropertyChangeEventPtr = QSharedPointer<PropertyChangeEvent>;

class FFMPEG_EXPORT Event : public QObject
{
    Q_OBJECT
public:
    enum class EventType {
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

    [[nodiscard]] virtual auto type() const -> EventType { return EventType::None; }
};

using EventPtr = QSharedPointer<Event>;

} // namespace Ffmpeg
