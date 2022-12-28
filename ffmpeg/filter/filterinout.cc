#include "filterinout.hpp"

extern "C" {
#include <libavfilter/avfilter.h>
}

namespace Ffmpeg {

class FilterInOut::FilterInOutPrivate
{
public:
    FilterInOutPrivate(QObject *parent)
        : owner(parent)
    {
        inOut = avfilter_inout_alloc();
    }
    ~FilterInOutPrivate() { avfilter_inout_free(&inOut); }

    QObject *owner;
    AVFilterInOut *inOut = nullptr;
};

FilterInOut::FilterInOut(QObject *parent)
    : QObject{parent}
    , d_ptr(new FilterInOutPrivate(this))
{}

FilterInOut::~FilterInOut() {}

AVFilterInOut *FilterInOut::avFilterInOut()
{
    return d_ptr->inOut;
}

} // namespace Ffmpeg
