#ifndef AVERROR_H
#define AVERROR_H

#include "ffmepg_global.h"

#include <QtCore>

namespace Ffmpeg {

class FFMPEG_EXPORT AVError
{
public:
    AVError(int error = 0) : m_error(error) {}

    void setError(int error) { m_error = error; }
    int error() const { return m_error; };

    QString errorString() const;

    static QString avErrorString(int error);

private:
    int m_error = 0;
    QString m_errorString;
};

}
#endif // AVERROR_H
