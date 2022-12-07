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

    bool isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) override;
    QSharedPointer<Frame> convertSupported_pix_fmt(QSharedPointer<Frame> frame) override;
    QVector<AVPixelFormat> supportedOutput_pix_fmt() override;

protected:
    void updateFrame(QSharedPointer<Frame> frame) override;

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void initVbo();
    void initTexture();
    void updateYUV420P();
    void updateYUYV422();
    void updateYUV422P();
    void updateYUV444P();
    void updateYUV410P();
    void updateYUV411P();
    void updateUYVY422();
    void updateRGB8(int dataType);
    void updateNV12();
    void updateRGB();
    void updateRGBA();
    void updateYUV420P10LE();
    void updateP010LE();
    void displayFrame(QSharedPointer<Frame> framePtr);

    class OpenglRenderPrivate;
    QScopedPointer<OpenglRenderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // OPENGLRENDER_HPP
