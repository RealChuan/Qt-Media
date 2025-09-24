#pragma once

#include "ffmepg_global.h"

#include <QImage>

#include <memory>
#include <vector>

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
    Frame &operator=(const Frame &other);
    Frame &operator=(Frame &&other) noexcept;
    ~Frame();

    auto compareProps(Frame *other) -> bool;
    void copyPropsFrom(Frame *src);
    auto imageAlloc(const QSize &size, AVPixelFormat pix_fmt = AV_PIX_FMT_RGBA, int align = 1)
        -> bool;
    void freeImageAlloc();
    void setPictType(AVPictureType type);
    void unref();
    void setPts(qint64 pts);
    auto pts() -> qint64;
    void setDuration(qint64 duration);
    auto duration() -> qint64;
    void destroyFrame();
    auto toImage() -> QImage; // maybe null
    auto getBuffer() -> bool;
    auto isKey() -> bool;
    auto avFrame() -> AVFrame *;

    static auto fromQImage(const QImage &image) -> Frame *;

private:
    class FramePrivate;
    std::unique_ptr<FramePrivate> d_ptr;
};

using FramePtr = std::shared_ptr<Frame>;
using FramePtrList = std::vector<FramePtr>;

} // namespace Ffmpeg
