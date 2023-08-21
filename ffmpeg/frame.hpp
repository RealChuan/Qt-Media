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
    Frame(Frame &&other);
    ~Frame();

    Frame &operator=(const Frame &other);
    Frame &operator=(Frame &&other);

    void copyPropsFrom(Frame *src);

    bool isKey();

    void unref();

    bool imageAlloc(const QSize &size, AVPixelFormat pix_fmt = AV_PIX_FMT_RGBA, int align = 1);
    void freeImageAlloc();

    void setPictType(AVPictureType type);

    void setPts(double pts);
    double pts();

    void setDuration(double duration);
    double duration();

    int format();

    void setAVFrameNull();

    QImage toImage(); // maybe null

    bool getBuffer();

    AVFrame *avFrame();

    static Frame *fromQImage(const QImage &image);

private:
    class FramePrivate;
    QScopedPointer<FramePrivate> d_ptr;
};

} // namespace Ffmpeg
