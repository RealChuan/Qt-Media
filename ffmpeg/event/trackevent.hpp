#pragma once

#include "event.hpp"

namespace Ffmpeg {

class FFMPEG_EXPORT TracksChangedEvent : public Event
{
public:
    TracksChangedEvent(const QStringList &tracks, EventType type, QObject *parent = nullptr)
        : Event(parent)
        , m_tracks(tracks)
    {
        setType(type);
    }

    void setType(EventType type)
    {
        Q_ASSERT(type == EventType::AudioTracksChanged || type == EventType::VideoTracksChanged
                 || type == EventType::SubtitleTracksChanged);
        m_type = type;
    }
    [[nodiscard]] auto type() const -> EventType override { return m_type; }

    void setTracks(const QStringList &tracks) { m_tracks = tracks; }
    [[nodiscard]] auto tracks() const -> QStringList { return m_tracks; }

private:
    EventType m_type = EventType::None;
    QStringList m_tracks;
};

class FFMPEG_EXPORT TrackChangedEvent : public Event
{
public:
    TrackChangedEvent(const QString &track, EventType type, QObject *parent = nullptr)
        : Event(parent)
        , m_track(track)
    {
        setType(type);
    }

    void setType(EventType type)
    {
        Q_ASSERT(type == EventType::AudioTrackChanged || type == EventType::VideoTrackChanged
                 || type == EventType::SubtitleTrackChanged);
        m_type = type;
    }
    [[nodiscard]] auto type() const -> EventType override { return m_type; }

    void setTrack(const QString &track) { m_track = track; }
    [[nodiscard]] auto track() const -> QString { return m_track; }

private:
    EventType m_type = EventType::None;
    QString m_track;
};

} // namespace Ffmpeg
