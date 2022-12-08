#ifndef SUBTITLE_H
#define SUBTITLE_H

#include <QImage>
#include <QObject>

struct AVSubtitle;
struct SwsContext;

namespace Ffmpeg {

class Subtitle : public QObject
{
    Q_OBJECT
public:
    explicit Subtitle(QObject *parent = nullptr);
    ~Subtitle();

    void setDefault(double pts, double duration, const QString &text);

    void parse(SwsContext *swsContext);

    AVSubtitle *avSubtitle();

    void clear();

    quint64 pts();

private:
    void parseImage(SwsContext *swsContext);
    void parseText();

    class SubtitlePrivate;
    QScopedPointer<SubtitlePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // SUBTITLE_H
