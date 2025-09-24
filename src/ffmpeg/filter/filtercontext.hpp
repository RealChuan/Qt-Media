#pragma once

#include <ffmpeg/frame.hpp>

#include <QObject>

struct AVFilterContext;

namespace Ffmpeg {

class FilterGraph;
class FilterContext : public QObject
{
public:
    // name list
    // Video: {buffer, buffersink}
    // Audio: {abuffer, abuffersink}
    explicit FilterContext(const QString &name, QObject *parent = nullptr);
    ~FilterContext() override;

    auto isValid() -> bool;

    auto create(const QString &name, const QString &args, FilterGraph *filterGraph) -> bool;

    auto buffersrcAddFrameFlags(const FramePtr &framePtr) -> bool;
    auto buffersinkGetFrame(const FramePtr &framePtr) -> bool;
    void buffersinkSetFrameSize(int size);

    auto avFilterContext() -> AVFilterContext *;

private:
    class FilterContextPrivate;
    QScopedPointer<FilterContextPrivate> d_ptr;
};

} // namespace Ffmpeg
