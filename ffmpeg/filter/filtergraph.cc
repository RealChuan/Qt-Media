#include "filtergraph.hpp"
#include "filterinout.hpp"

#include <ffmpeg/averrormanager.hpp>

#include <QDebug>

extern "C" {
#include <libavfilter/avfilter.h>
}

namespace Ffmpeg {

class FilterGraph::FilterGraphPrivate
{
public:
    explicit FilterGraphPrivate(FilterGraph *q)
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

FilterGraph::~FilterGraph() = default;

auto FilterGraph::parse(const QString &filters, FilterInOut *in, FilterInOut *out) -> bool
{
    auto *inputs = in->avFilterInOut();
    auto *outputs = out->avFilterInOut();
    auto ret = avfilter_graph_parse_ptr(d_ptr->filterGraph,
                                        filters.toLocal8Bit().constData(),
                                        &inputs,
                                        &outputs,
                                        nullptr);
    in->setAVFilterInOut(inputs);
    out->setAVFilterInOut(outputs);
    ERROR_RETURN(ret)
}

auto FilterGraph::config() -> bool
{
    auto ret = avfilter_graph_config(d_ptr->filterGraph, nullptr);
    ERROR_RETURN(ret)
}

auto FilterGraph::avFilterGraph() -> AVFilterGraph *
{
    return d_ptr->filterGraph;
}

} // namespace Ffmpeg
