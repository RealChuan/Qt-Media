#pragma once

#include "event.hpp"

#include <ffmpeg/averror.h>

namespace Ffmpeg {

class FFMPEG_EXPORT AVErrorEvent : public PropertyChangeEvent
{
public:
    explicit AVErrorEvent(const AVError &error, QObject *parent = nullptr)
        : PropertyChangeEvent(parent)
        , m_error(error)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::AVError; }

    void setError(const AVError &error) { m_error = error; }
    [[nodiscard]] auto error() const -> AVError { return m_error; }

private:
    AVError m_error;
};

class FFMPEG_EXPORT ErrorEvent : public PropertyChangeEvent
{
public:
    explicit ErrorEvent(const QString &error, QObject *parent = nullptr)
        : PropertyChangeEvent(parent)
        , m_error(error)
    {}

    [[nodiscard]] auto type() const -> EventType override { return EventType::Error; }

    void setError(const QString &error) { m_error = error; }
    [[nodiscard]] auto error() const -> QString { return m_error; }

private:
    QString m_error;
};

} // namespace Ffmpeg
