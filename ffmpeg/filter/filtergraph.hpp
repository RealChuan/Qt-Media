#ifndef FILTERGRAPH_HPP
#define FILTERGRAPH_HPP

#include "filter/filterinout.hpp"

struct AVFilterGraph;

namespace Ffmpeg {

class FilterInOut;
class FilterGraph : public QObject
{
public:
    explicit FilterGraph(QObject *parent = nullptr);
    ~FilterGraph() override;

    auto parse(const QString &filters, FilterInOut *in, FilterInOut *out) -> bool;

    auto config() -> bool;

    auto avFilterGraph() -> AVFilterGraph *;

private:
    class FilterGraphPrivate;
    QScopedPointer<FilterGraphPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // FILTERGRAPH_HPP
