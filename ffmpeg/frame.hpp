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

    AVFrame *avFrame();

private:
    AVFrame *m_frame;
    bool m_imageAlloc = false;
};

} // namespace Ffmpeg
