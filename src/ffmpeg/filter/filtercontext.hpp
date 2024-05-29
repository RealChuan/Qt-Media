#ifndef FILTERCONTEXT_HPP
#define FILTERCONTEXT_HPP

#include <QObject>

struct AVFilterContext;

namespace Ffmpeg {

class Frame;
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

    auto buffersrcAddFrameFlags(Frame *frame) -> bool;
    auto buffersinkGetFrame(Frame *frame) -> bool;
    void buffersinkSetFrameSize(int size);

    auto avFilterContext() -> AVFilterContext *;

private:
    class FilterContextPrivate;
    QScopedPointer<FilterContextPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // FILTERCONTEXT_HPP
