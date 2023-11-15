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

    auto isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) -> bool override;
    auto convertSupported_pix_fmt(QSharedPointer<Frame> frame) -> QSharedPointer<Frame> override;
    auto supportedOutput_pix_fmt() -> QVector<AVPixelFormat> override;

    void resetAllFrame() override;

    auto widget() -> QWidget * override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void updateFrame(QSharedPointer<Frame> frame) override;
    void updateSubTitleFrame(QSharedPointer<Subtitle> frame) override;

private:
    void clear();
    void draw();
    void initTexture();
    void initSubTexture();
    auto fitToScreen(const QSize &size) -> QMatrix4x4;
    void cleanup();
    void resetShader(int format);

    void onUpdateFrame(const QSharedPointer<Frame> &framePtr);
    void onUpdateSubTitleFrame(const QSharedPointer<Subtitle> &framePtr);

    void paintVideoFrame();
    void paintSubTitleFrame();
    void setColorTrc();

    void updateYUV420P();
    void updateYUYV422();
    void updateYUV422P();
    void updateYUV444P();
    void updateYUV410P();
    void updateYUV411P();
    void updateUYVY422();
    void updateRGB8(int dataType);
    void updateNV12(GLenum type);
    void updateRGB();
    void updateRGBA();

    class OpenglRenderPrivate;
    QScopedPointer<OpenglRenderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // OPENGLRENDER_HPP
