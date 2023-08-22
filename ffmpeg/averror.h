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
    AVError(AVError &&other) noexcept;
    ~AVError();

    auto operator=(const AVError &other) -> AVError &;
    auto operator=(AVError &&other) noexcept -> AVError &;

    void setErrorCode(int error);
    [[nodiscard]] auto errorCode() const -> int;

    [[nodiscard]] auto errorString() const -> QString;

    static auto avErrorString(int error) -> QString;

private:
    class AVErrorPrivate;
    QScopedPointer<AVErrorPrivate> d_ptr;
};

} // namespace Ffmpeg
#endif // AVERROR_H
