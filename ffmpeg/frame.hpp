#pragma once

#include "ffmepg_global.h"

#include <QEnableSharedFromThis>

extern "C" {
#include <libavutil/pixfmt.h>
}

class QSize;

struct AVFrame;

namespace Ffmpeg {

class FFMPEG_EXPORT Frame : public QEnableSharedFromThis<Frame>
{
public:
    explicit Frame();
    Frame(const Frame &other);
    Frame &operator=(const Frame &other);
    ~Frame();

    bool imageAlloc(const QSize &size, AVPixelFormat pix_fmt = AV_PIX_FMT_RGBA);
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
