#pragma once

#include "event.hpp"

#include <ffmpeg/mediainfo.hpp>

namespace Ffmpeg {

class FFMPEG_EXPORT MediaTrackEvent : public PropertyChangeEvent
{
public:
    explicit MediaTrackEvent(const QVector<StreamInfo> &tracks, QObject *parent = nullptr)
        : PropertyChangeEvent(parent)
        , m_tracks(tracks)
    {}
    
    [[nodiscard]] auto type() const -> EventType override { return MediaTrack; }

    void setTracks(const QVector<StreamInfo> &tracks) { m_tracks = tracks; }
    [[nodiscard]] auto tracks() const -> QVector<StreamInfo> { return m_tracks; }

private:
    QVector<StreamInfo> m_tracks;
};

} // namespace Ffmpeg
