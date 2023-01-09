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
    ~FilterContext();

    bool isValid();

    bool create(const QString &name, const QString &args, FilterGraph *filterGraph);

    bool buffersrc_addFrameFlags(Frame *frame);
    bool buffersink_getFrame(Frame *frame);
    void buffersink_setFrameSize(int size);

    AVFilterContext *avFilterContext();

private:
    void setError(int errorCode);

    class FilterContextPrivate;
    QScopedPointer<FilterContextPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // FILTERCONTEXT_HPP
