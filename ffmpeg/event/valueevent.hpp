#pragma once

#include "event.hpp"

#include <ffmpeg/mediainfo.hpp>

namespace Ffmpeg {

class FFMPEG_EXPORT DurationEvent : public PropertyChangeEvent
{
public:
    explicit DurationEvent(qint64 duration, QObject *parent = nullptr)
        : PropertyChangeEvent(parent)
        , m_duration(duration)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::Duration; }

    void setDuration(qint64 duration) { m_duration = duration; }
    [[nodiscard]] auto duration() const -> qint64 { return m_duration; }

private:
    qint64 m_duration = 0;
};

class FFMPEG_EXPORT PositionEvent : public PropertyChangeEvent
{
public:
    explicit PositionEvent(qint64 position, QObject *parent = nullptr)
        : PropertyChangeEvent(parent)
        , m_position(position)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::Position; }

    void setPosition(qint64 position) { m_position = position; }
    [[nodiscard]] auto position() const -> qint64 { return m_position; }

private:
    qint64 m_position = 0;
};

class FFMPEG_EXPORT MediaStateEvent : public PropertyChangeEvent
{
public:
    explicit MediaStateEvent(Ffmpeg::MediaState state, QObject *parent = nullptr)
        : PropertyChangeEvent(parent)
        , m_state(state)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::MediaState; }

    void setState(Ffmpeg::MediaState state) { m_state = state; }
    [[nodiscard]] auto state() const -> Ffmpeg::MediaState { return m_state; }

private:
    Ffmpeg::MediaState m_state = Ffmpeg::MediaState::Stopped;
};

class FFMPEG_EXPORT CacheSpeedEvent : public PropertyChangeEvent
{
public:
    explicit CacheSpeedEvent(qint64 speed, QObject *parent = nullptr)
        : PropertyChangeEvent(parent)
        , m_speed(speed)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::CacheSpeed; }

    void setSpeed(qint64 speed) { m_speed = speed; }
    [[nodiscard]] auto speed() const -> qint64 { return m_speed; }

private:
    qint64 m_speed = 0;
};

} // namespace Ffmpeg
