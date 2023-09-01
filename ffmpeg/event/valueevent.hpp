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

class PauseEvent : public Event
{
public:
    explicit PauseEvent(bool paused, QObject *parent = nullptr)
        : Event(parent)
        , m_paused(paused)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::Pause; }

    void setPaused(bool paused) { m_paused = paused; }
    [[nodiscard]] auto paused() const -> bool { return m_paused; }

private:
    bool m_paused = false;
};

class FFMPEG_EXPORT GpuEvent : public Event
{
public:
    explicit GpuEvent(bool use, QObject *parent = nullptr)
        : Event(parent)
        , m_use(use)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::Gpu; }

    void setUse(bool use) { m_use = use; }
    [[nodiscard]] auto use() const -> bool { return m_use; }

private:
    bool m_use = false;
};

class FFMPEG_EXPORT OpenMediaEvent : public Event
{
public:
    explicit OpenMediaEvent(const QString &filepath, QObject *parent = nullptr)
        : Event(parent)
        , m_filepath(filepath)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::OpenMedia; }

    void setFilePath(const QString &filepath) { m_filepath = filepath; }
    [[nodiscard]] auto filepath() const -> QString { return m_filepath; }

private:
    QString m_filepath;
};

class FFMPEG_EXPORT CloseMediaEvent : public Event
{
public:
    using Event::Event;

    [[nodiscard]] auto type() const -> EventType override { return EventType::CloseMedia; }
};

class FFMPEG_EXPORT SpeedEvent : public Event
{
public:
    explicit SpeedEvent(double speed, QObject *parent = nullptr)
        : Event(parent)
        , m_speed(speed)
    {
        Q_ASSERT(speed > 0);
    }

    [[nodiscard]] auto type() const -> EventType override { return EventType::Speed; }

    void setSpeed(double speed) { m_speed = speed; }
    [[nodiscard]] auto speed() const -> double { return m_speed; }

private:
    double m_speed = 0;
};

class FFMPEG_EXPORT VolumeEvent : public Event
{
public:
    explicit VolumeEvent(qreal volume, QObject *parent = nullptr)
        : Event(parent)
        , m_volume(volume)
    {
        Q_ASSERT(volume >= 0 && volume <= 1);
    }

    [[nodiscard]] auto type() const -> EventType override { return EventType::Volume; }

    void setVolume(qreal volume) { m_volume = volume; }
    [[nodiscard]] auto volume() const -> qreal { return m_volume; }

private:
    qreal m_volume = 0;
};

} // namespace Ffmpeg
