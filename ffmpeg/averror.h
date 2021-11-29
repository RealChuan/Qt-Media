#ifndef AVERROR_H
#define AVERROR_H

#include "ffmepg_global.h"

#include <QtCore>

namespace Ffmpeg {

class FFMPEG_EXPORT AVError
{
public:
    explicit AVError(int error = 0) { setError(error); }
    AVError(const AVError &other);
    AVError &operator=(const AVError &other);

    void setError(int error);
    int error() const { return m_error; }

    QString errorString() const;

    static QString avErrorString(int error);

private:
    int m_error = 0;
    QString m_errorString;
};

} // namespace Ffmpeg
#endif // AVERROR_H
