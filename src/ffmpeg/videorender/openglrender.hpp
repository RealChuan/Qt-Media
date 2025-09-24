#pragma once

#include "videorender.hpp"

#include <ffmpeg/frame.hpp>

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>

namespace Ffmpeg {

class FFMPEG_EXPORT OpenglRender : public VideoRender,
                                   public QOpenGLWidget,
                                   public QOpenGLFunctions_3_3_Core
{
public:
    explicit OpenglRender(QWidget *parent = nullptr);
    ~OpenglRender() override;

    auto isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) -> bool override;
    auto convertSupported_pix_fmt(const FramePtr &frame) -> FramePtr override;
    auto supportedOutput_pix_fmt() -> QList<AVPixelFormat> override;

    void resetAllFrame() override;

    auto widget() -> QWidget * override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void updateFrame(const FramePtr &framePtr) override;
    void updateSubTitleFrame(const QSharedPointer<Subtitle> &framePtr) override;

private:
    void clear();
    void draw();
    void initTexture();
    void initSubTexture();
    auto fitToScreen(const QSize &size) -> QMatrix4x4;
    void cleanup();
    void resetShader(const FramePtr &framePtr);

    void onUpdateFrame(const FramePtr &framePtr);
    void onUpdateSubTitleFrame(const QSharedPointer<Subtitle> &framePtr);

    void paintVideoFrame();
    void paintSubTitleFrame();

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
