#include "filtercontext.hpp"
#include "filtergraph.hpp"

#include <ffmpeg/averror.h>

#include <QDebug>

extern "C" {
#include <libavfilter/avfilter.h>
}

namespace Ffmpeg {

class FilterContext::FilterContextPrivate
{
public:
    FilterContextPrivate(QObject *parent)
        : owner(parent)
    {}

    QObject *owner;

    const AVFilter *filter = nullptr;
    AVFilterContext *filterContext = nullptr;
    AVError error;
};

FilterContext::FilterContext(const QString &name, QObject *parent)
    : QObject{parent}
    , d_ptr(new FilterContextPrivate(this))
{
    d_ptr->filter = avfilter_get_by_name(name.toLocal8Bit().constData());
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
