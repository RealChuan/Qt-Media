#include "filtercontext.hpp"
#include "averrormanager.hpp"
#include "filtergraph.hpp"

#include <ffmpeg/frame.hpp>

#include <QDebug>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}

namespace Ffmpeg {

class FilterContext::FilterContextPrivate
{
public:
    explicit FilterContextPrivate(FilterContext *q)
        : q_ptr(q)
    {}

    void createFilter(const QString &name)
    {
        filter = avfilter_get_by_name(name.toLocal8Bit().constData());
        Q_ASSERT(nullptr != filter);
    }

    FilterContext *q_ptr;

    const AVFilter *filter = nullptr;
    AVFilterContext *filterContext = nullptr;
};

FilterContext::FilterContext(const QString &name, QObject *parent)
    : QObject{parent}
    , d_ptr(new FilterContextPrivate(this))
{
    d_ptr->createFilter(name);
}

FilterContext::~FilterContext() = default;

auto FilterContext::isValid() -> bool
{
    return nullptr != d_ptr->filter;
}

auto FilterContext::create(const QString &name, const QString &args, FilterGraph *filterGraph)
    -> bool
{
    auto ret = avfilter_graph_create_filter(&d_ptr->filterContext,
                                            d_ptr->filter,
                                            name.toLocal8Bit().constData(),
                                            args.toLocal8Bit().constData(),
                                            nullptr,
                                            filterGraph->avFilterGraph());
    Q_ASSERT(d_ptr->filterContext != nullptr);
    ERROR_RETURN(ret)
}

auto FilterContext::buffersrcAddFrameFlags(Frame *frame) -> bool
{
    auto ret = av_buffersrc_add_frame_flags(d_ptr->filterContext,
                                            frame->avFrame(),
                                            AV_BUFFERSRC_FLAG_KEEP_REF | AV_BUFFERSRC_FLAG_PUSH);
    ERROR_RETURN(ret)
}

auto FilterContext::buffersinkGetFrame(Frame *frame) -> bool
{
    auto ret = av_buffersink_get_frame(d_ptr->filterContext, frame->avFrame());
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return false;
    }
    ERROR_RETURN(ret)
}

void FilterContext::buffersinkSetFrameSize(int size)
{
    av_buffersink_set_frame_size(d_ptr->filterContext, size);
}

auto FilterContext::avFilterContext() -> AVFilterContext *
{
    return d_ptr->filterContext;
}

} // namespace Ffmpeg
