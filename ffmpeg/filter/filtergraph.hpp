#ifndef FILTERGRAPH_HPP
#define FILTERGRAPH_HPP

#include "filter/filterinout.hpp"

struct AVFilterGraph;

namespace Ffmpeg {

class AVError;
class FilterInOut;
class FilterGraph : public QObject
{
    Q_OBJECT
public:
    explicit FilterGraph(QObject *parent = nullptr);
    ~FilterGraph();

    bool parse(const QString &filters, FilterInOut *in, FilterInOut *out);

    bool config();

    AVFilterGraph *avFilterGraph();

signals:
    void error(const Ffmpeg::AVError &avError);

private:
    void setError(int errorCode);

    class FilterGraphPrivate;
    QScopedPointer<FilterGraphPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // FILTERGRAPH_HPP
