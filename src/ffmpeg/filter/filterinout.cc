#include "filterinout.hpp"

extern "C" {
#include <libavfilter/avfilter.h>
}

namespace Ffmpeg {

class FilterInOut::FilterInOutPrivate
{
public:
    explicit FilterInOutPrivate(QObject *parent)
        : owner(parent)
    {
        inOut = avfilter_inout_alloc();
        Q_ASSERT(nullptr != inOut);
    }
    ~FilterInOutPrivate() { avfilter_inout_free(&inOut); }

    QObject *owner;
    AVFilterInOut *inOut = nullptr;
};

FilterInOut::FilterInOut(QObject *parent)
    : QObject{parent}
    , d_ptr(new FilterInOutPrivate(this))
{}

FilterInOut::~FilterInOut() = default;

auto FilterInOut::avFilterInOut() -> AVFilterInOut *
{
    return d_ptr->inOut;
}

void FilterInOut::setAVFilterInOut(AVFilterInOut *avFilterInOut)
{
    d_ptr->inOut = avFilterInOut;
}

} // namespace Ffmpeg
