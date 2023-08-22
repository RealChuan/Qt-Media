#include "filtergraph.hpp"
#include "filterinout.hpp"

#include <ffmpeg/averrormanager.hpp>

extern "C" {
#include <libavfilter/avfilter.h>
}

namespace Ffmpeg {

class FilterGraph::FilterGraphPrivate
{
public:
    FilterGraphPrivate(FilterGraph *q)
        : q_ptr(q)
    {
        filterGraph = avfilter_graph_alloc();
        Q_ASSERT(nullptr != filterGraph);
    }

    ~FilterGraphPrivate()
    {
        Q_ASSERT(nullptr != filterGraph);
        avfilter_graph_free(&filterGraph);
    }

    FilterGraph *q_ptr;

    AVFilterGraph *filterGraph = nullptr;
};

FilterGraph::FilterGraph(QObject *parent)
    : QObject{parent}
    , d_ptr(new FilterGraphPrivate(this))
{}

FilterGraph::~FilterGraph() {}

bool FilterGraph::parse(const QString &filters, FilterInOut *in, FilterInOut *out)
{
    auto inputs = in->avFilterInOut();
    auto outputs = out->avFilterInOut();
    auto ret = avfilter_graph_parse_ptr(d_ptr->filterGraph,
                                        filters.toLocal8Bit().constData(),
                                        &inputs,
                                        &outputs,
                                        nullptr);
    in->setAVFilterInOut(inputs);
    out->setAVFilterInOut(outputs);
    ERROR_RETURN(ret)
}

bool FilterGraph::config()
{
    auto ret = avfilter_graph_config(d_ptr->filterGraph, nullptr);
    ERROR_RETURN(ret)
}

AVFilterGraph *FilterGraph::avFilterGraph()
{
    return d_ptr->filterGraph;
}

} // namespace Ffmpeg
