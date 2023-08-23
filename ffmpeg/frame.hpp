#pragma once

#include "ffmepg_global.h"

#include <QImage>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>
}

struct AVFrame;

namespace Ffmpeg {

class FFMPEG_EXPORT Frame
{
public:
    Frame();
    Frame(const Frame &other);
    Frame(Frame &&other) noexcept;
    ~Frame();

    auto operator=(const Frame &other) -> Frame &;
    auto operator=(Frame &&other) noexcept -> Frame &;

    void copyPropsFrom(Frame *src);

    auto isKey() -> bool;

    void unref();

    auto imageAlloc(const QSize &size, AVPixelFormat pix_fmt = AV_PIX_FMT_RGBA, int align = 1)
        -> bool;
    void freeImageAlloc();

    void setPictType(AVPictureType type);

    void setPts(qint64 pts); // microseconds
    auto pts() -> qint64;

    void setDuration(qint64 duration); // microseconds
    auto duration() -> qint64;

    auto toImage() -> QImage; // maybe null

    auto getBuffer() -> bool;

    void destroyFrame();

    auto avFrame() -> AVFrame *;

    static auto fromQImage(const QImage &image) -> Frame *;

private:
    class FramePrivate;
    QScopedPointer<FramePrivate> d_ptr;
};

} // namespace Ffmpeg
