#include "filtergraph.hpp"
#include "filterinout.hpp"

#include <ffmpeg/averror.h>

extern "C" {
#include <libavfilter/avfilter.h>
}

namespace Ffmpeg {

class FilterGraph::FilterGraphPrivate
{
public:
    FilterGraphPrivate(QObject *parent)
        : owner(parent)
    {
        filterGraph = avfilter_graph_alloc();
    }
    ~FilterGraphPrivate() { avfilter_graph_free(&filterGraph); }

    QObject *owner;
    AVFilterGraph *filterGraph = nullptr;
    AVError error;
};

FilterGraph::FilterGraph(QObject *parent)
    : QObject{parent}
    , d_ptr(new FilterGraphPrivate(this))
{}

FilterGraph::~FilterGraph() {}

bool FilterGraph::parse(const QString &filters, FilterInOut *in, FilterInOut *out)
{
    auto outputs = out->avFilterInOut();
    auto inputs = in->avFilterInOut();
    auto ret = avfilter_graph_parse_ptr(d_ptr->filterGraph,
                                        filters.toLocal8Bit().constData(),
                                        &inputs,
                                        &outputs,
                                        nullptr);
    if (ret < 0) {
        setError(ret);
        return false;
    }
    return true;
}

bool FilterGraph::config()
{
    auto ret = avfilter_graph_config(d_ptr->filterGraph, nullptr);
    if (ret < 0) {
        setError(ret);
        return false;
    }
    return true;
}

AVFilterGraph *FilterGraph::avFilterGraph()
{
    return d_ptr->filterGraph;
}

void FilterGraph::setError(int errorCode)
{
    d_ptr->error.setError(errorCode);
    emit error(d_ptr->error);
}

} // namespace Ffmpeg
