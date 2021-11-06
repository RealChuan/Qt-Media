#include "averror.h"

extern "C" {
#include <libavutil/error.h>
}

namespace Ffmpeg {

AVError::AVError(const AVError &other)
{
    m_error = other.m_error;
    m_errorString = other.m_errorString;
}

AVError &AVError::operator=(const AVError &other)
{
    m_error = other.m_error;
    m_errorString = other.m_errorString;
    return *this;
}

void AVError::setError(int error)
{
    m_error = error;
    char buf[1024] = {0};
    av_strerror(m_error, buf, sizeof buf);
    m_errorString = buf;
}

QString AVError::errorString() const
{
    return m_errorString;
}

QString AVError::avErrorString(int error)
{
    return AVError(error).errorString();
}

} // namespace Ffmpeg
