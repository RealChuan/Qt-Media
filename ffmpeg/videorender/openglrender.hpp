#ifndef OPENGLRENDER_HPP
#define OPENGLRENDER_HPP

#include "videorender.hpp"

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>

struct AVFrame;

namespace Ffmpeg {

class FFMPEG_EXPORT OpenglRender : public VideoRender,
                                   public QOpenGLWidget,
                                   public QOpenGLFunctions_3_3_Core
{
public:
    explicit OpenglRender(QWidget *parent = nullptr);
    ~OpenglRender() override;

    void setFrame(Frame *frame) override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void initVbo();
    void initTexture();
    void updateYUV420P();
    void updateNV12();
    void updateFrame(Frame *frame);

    class OpenglRenderPrivate;
    QScopedPointer<OpenglRenderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // OPENGLRENDER_HPP
