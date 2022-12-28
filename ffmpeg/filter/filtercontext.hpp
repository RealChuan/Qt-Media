#ifndef FILTERCONTEXT_HPP
#define FILTERCONTEXT_HPP

#include <QObject>

struct AVFilterContext;

namespace Ffmpeg {

class AVError;
class FilterGraph;
class FilterContext : public QObject
{
    Q_OBJECT
public:
    // name list
    // {buffer, buffersink, abuffer, abuffersink}
    explicit FilterContext(const QString &name, QObject *parent = nullptr);
    ~FilterContext();

    bool isValid();

    bool create(const QString &name, const QString &args, FilterGraph *filterGraph);

    AVFilterContext *avFilterContext();

signals:
    void error(const Ffmpeg::AVError &avError);

private:
    void setError(int errorCode);

    class FilterContextPrivate;
    QScopedPointer<FilterContextPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // FILTERCONTEXT_HPP
