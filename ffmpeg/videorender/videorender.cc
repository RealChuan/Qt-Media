#include "videorender.hpp"

#include <ffmpeg/frame.hpp>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

VideoRender::VideoRender() {}

VideoRender::~VideoRender() {}

void VideoRender::setFrame(QSharedPointer<Frame> frame)
{
    if (!isSupportedOutput_pix_fmt(AVPixelFormat(frame->avFrame()->format))) {
        convertSupported_pix_fmt(frame);
    }
    updateFrame(frame);
}

} // namespace Ffmpeg
