#pragma once

#include "event.hpp"

#include <latch>

namespace Ffmpeg {

class FFMPEG_EXPORT SeekEvent : public Event
{
public:
    explicit SeekEvent(qint64 position, QObject *parent = nullptr)
        : Event(parent)
        , m_position(position)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::Seek; }

    void setPosition(qint64 position) { m_position = position; }
    [[nodiscard]] auto position() const -> qint64 { return m_position; }

    void setWaitCountdown(int count) { m_latchPtr = std::make_unique<std::latch>(count); }
    void countDown()
    {
        if (m_latchPtr)
            m_latchPtr->count_down();
    }
    void wait()
    {
        if (m_latchPtr)
            m_latchPtr->wait();
    }

private:
    qint64 m_position = 0;
    std::unique_ptr<std::latch> m_latchPtr;
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
