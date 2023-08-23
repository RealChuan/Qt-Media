#ifndef ASS_HPP
#define ASS_HPP

#include "assdata.hpp"

#include <QObject>

namespace Ffmpeg {

class Ass : public QObject
{
    Q_OBJECT
public:
    explicit Ass(QObject *parent = nullptr);
    ~Ass();

    void init(uint8_t *extradata, int extradata_size);

    void setWindowSize(const QSize &size);

    void setFont(const QString &fontFamily);
    void addFont(const QByteArray &name, const QByteArray &data);

    void addSubtitleEvent(const QByteArray &data);
    void addSubtitleEvent(const QByteArray &data, qint64 pts, qint64 duration);
    void addSubtitleChunk(const QByteArray &data, qint64 pts, qint64 duration);

    void getRGBAData(AssDataInfoList &list, qint64 pts);

    void flushASSEvents();

private:
    class AssPrivate;
    QScopedPointer<AssPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // ASS_HPP
