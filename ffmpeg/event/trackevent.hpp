#pragma once

#include "event.hpp"

#include <ffmpeg/mediainfo.hpp>

namespace Ffmpeg {

class FFMPEG_EXPORT TracksChangedEvent : public Event
{
public:
    explicit TracksChangedEvent(const QVector<StreamInfo> &tracks, QObject *parent = nullptr)
        : Event(parent)
        , m_tracks(tracks)
    {}

    [[nodiscard]] auto type() const -> EventType override { return MediaTracksChanged; }

    void setTracks(const QVector<StreamInfo> &tracks) { m_tracks = tracks; }
    [[nodiscard]] auto tracks() const -> QVector<StreamInfo> { return m_tracks; }

private:
    QVector<StreamInfo> m_tracks;
};

} // namespace Ffmpeg
