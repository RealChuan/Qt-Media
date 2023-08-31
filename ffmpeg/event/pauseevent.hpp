#pragma once

#include "event.hpp"

namespace Ffmpeg {

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

} // namespace Ffmpeg
