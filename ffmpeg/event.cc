#include "event.hpp"

namespace Ffmpeg {

class Event::EventPrivate
{
public:
    explicit EventPrivate(Event *q)
        : q_ptr(q)
    {}

    Event *q_ptr;

    EventType type;
    QVariantList args;
};

Event::Event(EventType type, const QVariantList &args, QObject *parent)
    : QObject(parent)
    , d_ptr(new EventPrivate(this))
{
    d_ptr->type = type;
    d_ptr->args = std::move(args);
}

Event::~Event() = default;

auto Event::type() const -> EventType
{
    return d_ptr->type;
}

auto Event::args() const -> QVariantList
{
    return d_ptr->args;
}

} // namespace Ffmpeg
