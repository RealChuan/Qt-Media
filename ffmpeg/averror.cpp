#include "averror.h"

extern "C" {
#include <libavutil/error.h>
}

namespace Ffmpeg {

class AVError::AVErrorPrivate
{
public:
    AVErrorPrivate() {}

    int error = 0;
    QString errorString;
};

AVError::AVError(int error)
    : d_ptr(new AVErrorPrivate)
{
    setErrorCode(error);
}

AVError::AVError(const AVError &other)
    : d_ptr(new AVErrorPrivate)
{
    d_ptr->error = other.d_ptr->error;
    d_ptr->errorString = other.d_ptr->errorString;
}

AVError &AVError::operator=(const AVError &other)
{
    d_ptr->error = other.d_ptr->error;
    d_ptr->errorString = other.d_ptr->errorString;
    return *this;
}

AVError::~AVError() {}

void AVError::setErrorCode(int error)
{
    d_ptr->error = error;
    char buf[1024] = {0};
    av_strerror(d_ptr->error, buf, sizeof buf);
    d_ptr->errorString = buf;
}

int AVError::errorCode() const
{
    return d_ptr->error;
}

QString AVError::errorString() const
{
    return d_ptr->errorString;
}

QString AVError::avErrorString(int error)
{
    return AVError(error).errorString();
}

} // namespace Ffmpeg
