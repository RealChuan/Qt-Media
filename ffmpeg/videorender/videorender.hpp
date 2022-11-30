#ifndef VIDEORENDER_HPP
#define VIDEORENDER_HPP

#include <ffmpeg/ffmepg_global.h>

namespace Ffmpeg {

class Frame;

class FFMPEG_EXPORT VideoRender
{
    Q_DISABLE_COPY_MOVE(VideoRender)
public:
    VideoRender();
    virtual ~VideoRender();

    virtual void setFrame(Frame *frame) = 0;
};

} // namespace Ffmpeg

#endif // VIDEORENDER_HPP
