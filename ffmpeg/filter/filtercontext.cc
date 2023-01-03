#include "filtercontext.hpp"
#include "filtergraph.hpp"

#include <ffmpeg/averror.h>
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
    FilterContextPrivate(QObject *parent)
        : owner(parent)
    {}

    void createFilter(const QString &name)
    {
        filter = avfilter_get_by_name(name.toLocal8Bit().constData());
        Q_ASSERT(nullptr != filter);
    }

    QObject *owner;

    const AVFilter *filter = nullptr;
    AVFilterContext *filterContext = nullptr;
    AVError error;
};

FilterContext::FilterContext(const QString &name, QObject *parent)
    : QObject{parent}
    , d_ptr(new FilterContextPrivate(this))
{
    d_ptr->createFilter(name);
}

FilterContext::~FilterContext() {}

bool FilterContext::isValid()
{
    return nullptr != d_ptr->filter;
}

bool FilterContext::create(const QString &name, const QString &args, FilterGraph *filterGraph)
{
    auto ret = avfilter_graph_create_filter(&d_ptr->filterContext,
                                            d_ptr->filter,
                                            name.toLocal8Bit().constData(),
                                            args.toLocal8Bit().constData(),
                                            nullptr,
                                            filterGraph->avFilterGraph());
    if (ret < 0) {
        setError(ret);
        return false;
    }
    return true;
}

bool FilterContext::buffersrc_addFrameFlags(Frame *frame)
{
    auto ret = av_buffersrc_add_frame_flags(d_ptr->filterContext, frame->avFrame(), 0);
    if (ret < 0) {
        setError(ret);
        return false;
    }
    return true;
}

bool FilterContext::buffersink_getFrame(Frame *frame)
{
    auto ret = av_buffersink_get_frame(d_ptr->filterContext, frame->avFrame());
    if (ret < 0) {
        setError(ret);
        return false;
    }
    return true;
}

AVFilterContext *FilterContext::avFilterContext()
{
    return d_ptr->filterContext;
}

void FilterContext::setError(int errorCode)
{
    d_ptr->error.setError(errorCode);
    emit error(d_ptr->error);
}

} // namespace Ffmpeg
