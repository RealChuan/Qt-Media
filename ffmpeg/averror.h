#ifndef AVERROR_H
#define AVERROR_H

#include "ffmepg_global.h"

#include <QtCore>

namespace Ffmpeg {

class FFMPEG_EXPORT AVError
{
public:
    explicit AVError(int error = 0);
    AVError(const AVError &other);
    AVError &operator=(const AVError &other);
    ~AVError();

    void setErrorCode(int error);
    int errorCode() const;

    QString errorString() const;

    static QString avErrorString(int error);

private:
    class AVErrorPrivate;
    QScopedPointer<AVErrorPrivate> d_ptr;
};

} // namespace Ffmpeg
#endif // AVERROR_H
