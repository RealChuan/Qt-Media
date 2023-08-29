#pragma once

#include "event.hpp"

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

private:
    qint64 m_position = 0;
};

} // namespace Ffmpeg
