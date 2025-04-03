#pragma once

#include "event.hpp"

#include <ffmpeg/mediainfo.hpp>

namespace Ffmpeg {

class FFMPEG_EXPORT MediaTrackEvent : public PropertyChangeEvent
{
public:
    explicit MediaTrackEvent(const QList<StreamInfo> &tracks, QObject *parent = nullptr)
        : PropertyChangeEvent(parent)
        , m_tracks(tracks)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::MediaTrack; }

    void setTracks(const QList<StreamInfo> &tracks) { m_tracks = tracks; }
    [[nodiscard]] auto tracks() const -> QList<StreamInfo> { return m_tracks; }

private:
    QList<StreamInfo> m_tracks;
};

class FFMPEG_EXPORT SelectedMediaTrackEvent : public Event
{
public:
    explicit SelectedMediaTrackEvent(int index, EventType type, QObject *parent = nullptr)
        : Event(parent)
        , m_index(index)
        , m_type(type)
    {
        Q_ASSERT(type == EventType::AudioTarck || type == EventType::VideoTrack
                 || type == EventType::SubtitleTrack);
    }

    [[nodiscard]] auto type() const -> EventType override { return m_type; }

    void setIndex(int index) { m_index = index; }
    [[nodiscard]] auto index() const -> int { return m_index; }

private:
    int m_index;
    EventType m_type;
};

} // namespace Ffmpeg
