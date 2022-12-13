#pragma once

#include "ffmepg_global.h"

extern "C" {
#include <libavutil/pixfmt.h>
}

class QSize;
class QImage;

struct AVFrame;

namespace Ffmpeg {

class FFMPEG_EXPORT Frame
{
public:
    explicit Frame();
    Frame(const QImage &image);

    Frame(const Frame &other);
    Frame &operator=(const Frame &other);
    ~Frame();

    bool isKey();

    bool imageAlloc(const QSize &size, AVPixelFormat pix_fmt = AV_PIX_FMT_RGBA, int align = 1);
    void freeimageAlloc();

    void clear();

    void setPts(double pts);
    double pts();

    void setDuration(double duration);
    double duration();

    AVFrame *avFrame();

private:
    AVFrame *m_frame;
    bool m_imageAlloc = false;
    double m_pts = 0;
    double m_duration = 0;
};

} // namespace Ffmpeg
