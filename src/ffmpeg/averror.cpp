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
    QString funcInfo;
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
    d_ptr->funcInfo = other.d_ptr->funcInfo;
}

AVError::AVError(AVError &&other) noexcept
    : d_ptr(new AVErrorPrivate(this))
{
    d_ptr->error = other.d_ptr->error;
    d_ptr->errorString = other.d_ptr->errorString;
    d_ptr->funcInfo = other.d_ptr->funcInfo;
}

AVError::~AVError() = default;

auto AVError::operator=(const AVError &other) -> AVError &
{
    d_ptr->error = other.d_ptr->error;
    d_ptr->errorString = other.d_ptr->errorString;
    d_ptr->funcInfo = other.d_ptr->funcInfo;
    return *this;
}

auto AVError::operator=(AVError &&other) noexcept -> AVError &
{
    d_ptr->error = other.d_ptr->error;
    d_ptr->errorString = other.d_ptr->errorString;
    d_ptr->funcInfo = other.d_ptr->funcInfo;
    return *this;
}

auto AVError::setFuncInfo(const QString &funcInfo) -> AVError &
{
    d_ptr->funcInfo = funcInfo;
    return *this;
}

auto AVError::setErrorCode(int error) -> AVError &
{
    d_ptr->error = error;
    if (error < 0) {
        char buffer[AV_ERROR_MAX_STRING_SIZE + 1] = {};
        av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, error);
        d_ptr->errorString = QString::fromLocal8Bit(buffer);
    } else {
        d_ptr->errorString.clear();
    }
    return *this;
}

auto AVError::errorCode() const -> int
{
    return d_ptr->error;
}

auto AVError::errorString() const -> QString
{
    QString errorString;
    if (d_ptr->funcInfo.isEmpty()) {
        errorString = d_ptr->errorString;
    } else {
        errorString = QString("%1: %2").arg(d_ptr->errorString, d_ptr->funcInfo);
    }
    return errorString;
}

auto AVError::avErrorString(int error) -> QString
{
    return AVError(error).errorString();
}

} // namespace Ffmpeg
