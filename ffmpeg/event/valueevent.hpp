#pragma once

#include "event.hpp"

#include <ffmpeg/mediainfo.hpp>

namespace Ffmpeg {

class FFMPEG_EXPORT DurationChangedEvent : public Event
{
public:
    explicit DurationChangedEvent(qint64 duration, QObject *parent = nullptr)
        : Event(parent)
        , m_duration(duration)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::DurationChanged; }

    void setDuration(qint64 duration) { m_duration = duration; }
    [[nodiscard]] auto duration() const -> qint64 { return m_duration; }

private:
    qint64 m_duration = 0;
};

class FFMPEG_EXPORT PositionChangedEvent : public Event
{
public:
    explicit PositionChangedEvent(qint64 position, QObject *parent = nullptr)
        : Event(parent)
        , m_position(position)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::PositionChanged; }

    void setPosition(qint64 position) { m_position = position; }
    [[nodiscard]] auto position() const -> qint64 { return m_position; }

private:
    qint64 m_position = 0;
};

class FFMPEG_EXPORT MediaStateChangedEvent : public Event
{
public:
    explicit MediaStateChangedEvent(MediaState state, QObject *parent = nullptr)
        : Event(parent)
        , m_state(state)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::MediaStateChanged; }

    void setState(MediaState state) { m_state = state; }
    [[nodiscard]] auto state() const -> MediaState { return m_state; }

private:
    MediaState m_state = MediaState::Stopped;
};

class FFMPEG_EXPORT CacheSpeedChangedEvent : public Event
{
public:
    explicit CacheSpeedChangedEvent(qint64 speed, QObject *parent = nullptr)
        : Event(parent)
        , m_speed(speed)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::CacheSpeedChanged; }

    void setSpeed(qint64 speed) { m_speed = speed; }
    [[nodiscard]] auto speed() const -> qint64 { return m_speed; }

private:
    qint64 m_speed = 0;
};

} // namespace Ffmpeg
