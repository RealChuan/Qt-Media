#ifndef SUBTITLE_H
#define SUBTITLE_H

#include <QImage>
#include <QObject>

struct AVSubtitle;

namespace Ffmpeg {

struct SubtitleImage{
    QRectF rectF;
    QImage image;
    QString text;
    qint64 startDisplayTime; // ms
    qint64 endDisplayTime; // ms
};

class Subtitle : public QObject
{
    Q_OBJECT
public:
    explicit Subtitle(QObject *parent = nullptr);
    ~Subtitle();

    void setdefault(double pts, double duration, const QString &text);

    QVector<SubtitleImage> subtitleImages();

    AVSubtitle *avSubtitle();

    void clear();

private:
    QVector<SubtitleImage> scale();
    QVector<SubtitleImage> text();

    class SubtitlePrivate;
    QScopedPointer<SubtitlePrivate> d_ptr;
};

}

#endif // SUBTITLE_H
