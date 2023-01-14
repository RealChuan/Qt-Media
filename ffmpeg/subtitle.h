#ifndef SUBTITLE_H
#define SUBTITLE_H

#include "ffmepg_global.h"

#include <ffmpeg/subtitle/assdata.hpp>

#include <QObject>

struct AVSubtitle;
struct SwsContext;

namespace Ffmpeg {

class Ass;

class FFMPEG_EXPORT Subtitle : public QObject
{
    Q_OBJECT
public:
    enum Type { Unknown, Graphics, TEXT, ASS };

    explicit Subtitle(QObject *parent = nullptr);
    ~Subtitle();

    void setDefault(double pts, double duration, const QString &text);

    void parse(SwsContext *swsContext);
    QByteArrayList texts() const;

    void setVideoResolutionRatio(const QSize &size);
    QSize videoResolutionRatio() const;

    bool resolveAss(Ass *ass);
    void setAssDataInfoList(const AssDataInfoList &list);
    AssDataInfoList list() const;

    QImage generateImage() const;
    QImage image() const;

    void clear();

    Type type() const;

    double pts();
    double duration();

    AVSubtitle *avSubtitle();

private:
    class SubtitlePrivate;
    QScopedPointer<SubtitlePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // SUBTITLE_H
