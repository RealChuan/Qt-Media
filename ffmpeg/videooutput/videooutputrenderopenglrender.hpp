#ifndef VIDEOOUTPUTRENDEROPENGLRENDER_HPP
#define VIDEOOUTPUTRENDEROPENGLRENDER_HPP

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

#include "videooutputrender.hpp"

namespace Ffmpeg {

class FFMPEG_EXPORT VideoOutputRenderOpenGLRender : public VideoOutputRender,
                                                    public QOpenGLWidget,
                                                    protected QOpenGLFunctions
{
public:
    explicit VideoOutputRenderOpenGLRender(QWidget *parent = nullptr);
    ~VideoOutputRenderOpenGLRender();

    void setDisplayImage(const QImage &image) override;

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

private:
    void initVbo();
    void initShader();
    void initTexture();

    class VideoOutputRenderOpenGLRenderPrivate;
    QScopedPointer<VideoOutputRenderOpenGLRenderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // VIDEOOUTPUTRENDEROPENGLRENDER_HPP
