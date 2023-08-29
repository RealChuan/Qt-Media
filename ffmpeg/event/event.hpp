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
        AudioTracksChanged,
        VideoTracksChanged,
        SubtitleTracksChanged,
        AudioTrackChanged,
        VideoTrackChanged,
        SubtitleTrackChanged,
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

    bool operator==(const Event &other) const { return type() == other.type(); }
    bool operator!=(const Event &other) const { return !(*this == other); }

    [[nodiscard]] virtual auto type() const -> EventType { return None; }
};

using EventPtr = QSharedPointer<Event>;

} // namespace Ffmpeg
