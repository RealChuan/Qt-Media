#ifndef VIDEORENDERCREATE_HPP
#define VIDEORENDERCREATE_HPP

#include "videorender.hpp"

namespace Ffmpeg {

namespace VideoRenderCreate {

enum RenderType { Widget = 1, Opengl };

FFMPEG_EXPORT auto create(RenderType type) -> VideoRender *;

FFMPEG_EXPORT void setSurfaceFormatVersion(int major, int minor);

} // namespace VideoRenderCreate

} // namespace Ffmpeg

#endif // VIDEORENDERCREATE_HPP
