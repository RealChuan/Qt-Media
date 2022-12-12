#ifndef SUBTITLE_H
#define SUBTITLE_H

#include <QObject>

#include <subtitle/assdata.hpp>

struct AVSubtitle;
struct SwsContext;

namespace Ffmpeg {

class Ass;

class Subtitle : public QObject
{
    Q_OBJECT
public:
    enum Type { Unknown, Graphics, TEXT, ASS };

    explicit Subtitle(QObject *parent = nullptr);
    ~Subtitle();

    void setDefault(double pts, double duration, const QString &text);

    void parse(SwsContext *swsContext);
    QByteArrayList texts() const;

    AVSubtitle *avSubtitle();

    void clear();

    double pts();
    double duration();

    Type type() const;

    void setVideoResolutionRatio(const QSize &size);
    QSize videoResolutionRatio() const;

    bool resolveAss(Ass *ass);
    void setAssDataInfoList(const AssDataInfoList &list);
    AssDataInfoList list() const;

private:
    void parseImage(SwsContext *swsContext);
    void parseText();

    class SubtitlePrivate;
    QScopedPointer<SubtitlePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // SUBTITLE_H
