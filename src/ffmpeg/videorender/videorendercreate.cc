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

void setSurfaceFormatVersion(int major, int minor)
{
    auto surfaceFormat = QSurfaceFormat::defaultFormat();
    surfaceFormat.setVersion(major, minor);
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(surfaceFormat);
}

} // namespace VideoRenderCreate

} // namespace Ffmpeg
