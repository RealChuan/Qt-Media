#include "averror.h"

extern "C" {
#include <libavutil/error.h>
}

namespace Ffmpeg {

class AVError::AVErrorPrivate
{
public:
    explicit AVErrorPrivate(AVError *q)
        : q_ptr(q)
    {}

    AVError *q_ptr;
    int error = 0;
    QString errorString;
};

AVError::AVError(int error)
    : d_ptr(new AVErrorPrivate(this))
{
    setErrorCode(error);
}

AVError::AVError(const AVError &other)
    : d_ptr(new AVErrorPrivate(this))
{
    d_ptr->error = other.d_ptr->error;
    d_ptr->errorString = other.d_ptr->errorString;
}

AVError::AVError(AVError &&other) noexcept
    : d_ptr(new AVErrorPrivate(this))
{
    d_ptr->error = other.d_ptr->error;
    d_ptr->errorString = std::move(other.d_ptr->errorString);
}

AVError::~AVError() = default;

auto AVError::operator=(const AVError &other) -> AVError &
{
    d_ptr->error = other.d_ptr->error;
    d_ptr->errorString = other.d_ptr->errorString;
    return *this;
}

auto AVError::operator=(AVError &&other) noexcept -> AVError &
{
    d_ptr->error = other.d_ptr->error;
    d_ptr->errorString = std::move(other.d_ptr->errorString);
    return *this;
}

void AVError::setErrorCode(int error)
{
    d_ptr->error = error;
    if (error < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(error, errbuf, sizeof(errbuf));
        d_ptr->errorString = QString::fromUtf8(errbuf);
    } else {
        d_ptr->errorString.clear();
    }
}

auto AVError::errorCode() const -> int
{
    return d_ptr->error;
}

auto AVError::errorString() const -> QString
{
    return d_ptr->errorString;
}

auto AVError::avErrorString(int error) -> QString
{
    return AVError(error).errorString();
}

} // namespace Ffmpeg
