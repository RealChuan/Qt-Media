#include "videorendercreate.hpp"
#include "openglrender.hpp"
#include "widgetrender.hpp"

namespace Ffmpeg {

namespace VideoRenderCreate {

auto create(RenderType type) -> VideoRender *
{
    VideoRender *render = nullptr;
    switch (type) {
    //case RenderType::Widget: render = new WidgetRender; break;
    case RenderType::Opengl: render = new OpenglRender; break;
    default: render = new WidgetRender; break;
    }
    return render;
}

} // namespace VideoRenderCreate

} // namespace Ffmpeg
