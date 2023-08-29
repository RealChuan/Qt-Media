#pragma once

#include "event.hpp"

#include <ffmpeg/averror.h>

namespace Ffmpeg {

class FFMPEG_EXPORT ErrorEvent : public Event
{
public:
    explicit ErrorEvent(const AVError &error, QObject *parent = nullptr)
        : Event(parent)
        , m_error(error)
    {}

    [[nodiscard]] auto type() const -> EventType override { return Error; }

    void setError(const AVError &error) { m_error = error; }
    [[nodiscard]] auto error() const -> AVError { return m_error; }

private:
    AVError m_error;
};

} // namespace Ffmpeg
