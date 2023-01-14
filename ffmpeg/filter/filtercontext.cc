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
    FilterContextPrivate(QObject *parent)
        : owner(parent)
    {}

    void createFilter(const QString &name)
    {
        filter = avfilter_get_by_name(name.toLocal8Bit().constData());
        Q_ASSERT(nullptr != filter);
    }

    void setError(int errorCode) { AVErrorManager::instance()->setErrorCode(errorCode); }

    QObject *owner;

    const AVFilter *filter = nullptr;
    AVFilterContext *filterContext = nullptr;
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
        d_ptr->setError(ret);
        return false;
    }
    return true;
}

bool FilterContext::buffersrc_addFrameFlags(Frame *frame)
{
    auto ret = av_buffersrc_add_frame_flags(d_ptr->filterContext,
                                            frame->avFrame(),
                                            AV_BUFFERSRC_FLAG_KEEP_REF | AV_BUFFERSRC_FLAG_PUSH);
    if (ret < 0) {
        d_ptr->setError(ret);
        return false;
    }
    return true;
}

bool FilterContext::buffersink_getFrame(Frame *frame)
{
    auto ret = av_buffersink_get_frame(d_ptr->filterContext, frame->avFrame());
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return false;
    }
    if (ret < 0) {
        d_ptr->setError(ret);
        return false;
    }
    return true;
}

void FilterContext::buffersink_setFrameSize(int size)
{
    av_buffersink_set_frame_size(d_ptr->filterContext, size);
}

AVFilterContext *FilterContext::avFilterContext()
{
    return d_ptr->filterContext;
}

} // namespace Ffmpeg
