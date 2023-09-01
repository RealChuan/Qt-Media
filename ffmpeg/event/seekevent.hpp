#pragma once

#include "event.hpp"

#include <utils/countdownlatch.hpp>

namespace Ffmpeg {

class FFMPEG_EXPORT SeekEvent : public Event
{
public:
    explicit SeekEvent(qint64 position, QObject *parent = nullptr)
        : Event(parent)
        , m_position(position)
        , m_latch(0)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::Seek; }

    void setPosition(qint64 position) { m_position = position; }
    [[nodiscard]] auto position() const -> qint64 { return m_position; }

    void setWaitCountdown(qint64 count) { m_latch.setCount(count); }
    void countDown() { m_latch.countDown(); }
    void wait() { m_latch.wait(); }

private:
    qint64 m_position = 0;
    Utils::CountDownLatch m_latch;
};

class FFMPEG_EXPORT SeekRelativeEvent : public Event
{
public:
    explicit SeekRelativeEvent(qint64 relativePosition, QObject *parent = nullptr)
        : Event(parent)
        , m_relativePosition(relativePosition)
    {}
    
    [[nodiscard]] auto type() const -> EventType override { return EventType::SeekRelative; }

    // second
    void setRelativePosition(qint64 relativePosition) { m_relativePosition = relativePosition; }
    [[nodiscard]] auto relativePosition() const -> qint64 { return m_relativePosition; }

private:
    qint64 m_relativePosition = 0;
};

class FFMPEG_EXPORT SeekChangedEvent : public PropertyChangeEvent
{
public:
    explicit SeekChangedEvent(qint64 position, QObject *parent = nullptr)
        : PropertyChangeEvent(parent)
        , m_position(position)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::SeekChanged; }

    void setPosition(qint64 position) { m_position = position; }
    [[nodiscard]] auto position() const -> qint64 { return m_position; }

private:
    qint64 m_position = 0;
};

} // namespace Ffmpeg
