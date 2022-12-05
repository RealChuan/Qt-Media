#include "openglrender.hpp"

#include <ffmpeg/frame.hpp>
#include <ffmpeg/videoframeconverter.hpp>

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

class OpenglRender::OpenglRenderPrivate
{
public:
    OpenglRenderPrivate(QWidget *parnet)
        : owner(parnet)
    {}
    ~OpenglRenderPrivate() {}

    QWidget *owner;

    QScopedPointer<QOpenGLShaderProgram> programPtr;
    GLuint textureY;
    GLuint textureU;
    GLuint textureV;
    GLuint textureUV;
    GLuint textureRGBA;
    QOpenGLBuffer vbo = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    QOpenGLBuffer ebo = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    GLuint vao = 0; // 顶点数组对象,任何随后的顶点属性调用都会储存在这个VAO中，一个VAO可以有多个VBO

    QSizeF size;
    QRectF frameRect;
    QSharedPointer<Frame> framePtr;
    QVector<AVPixelFormat> supportFormats = {AV_PIX_FMT_YUV420P,
                                             AV_PIX_FMT_YUYV422,
                                             AV_PIX_FMT_RGB24,
                                             AV_PIX_FMT_BGR24,
                                             AV_PIX_FMT_YUV422P,
                                             AV_PIX_FMT_YUV444P,
                                             AV_PIX_FMT_NV12,
                                             AV_PIX_FMT_ARGB,
                                             AV_PIX_FMT_RGBA,
                                             AV_PIX_FMT_ABGR,
                                             AV_PIX_FMT_BGRA,
                                             AV_PIX_FMT_0RGB,
                                             AV_PIX_FMT_RGB0,
                                             AV_PIX_FMT_0BGR,
                                             AV_PIX_FMT_BGR0};
    QScopedPointer<VideoFrameConverter> frameConverterPtr;

    QColor backgroundColor = Qt::black;
};

OpenglRender::OpenglRender(QWidget *parent)
    : QOpenGLWidget(parent)
    , d_ptr(new OpenglRenderPrivate(this))
{}

OpenglRender::~OpenglRender()
{
    if (isValid()) {
        return;
    }
    makeCurrent();
    d_ptr->programPtr.reset();
    d_ptr->vbo.destroy();
    d_ptr->ebo.destroy();
    doneCurrent();
    glDeleteVertexArrays(1, &d_ptr->vao);
    glDeleteTextures(1, &d_ptr->textureY);
    glDeleteTextures(1, &d_ptr->textureU);
    glDeleteTextures(1, &d_ptr->textureV);
    glDeleteTextures(1, &d_ptr->textureUV);
}

bool OpenglRender::isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt)
{
    return d_ptr->supportFormats.contains(pix_fmt);
}

QSharedPointer<Frame> OpenglRender::convertSupported_pix_fmt(QSharedPointer<Frame> frame)
{
    auto avframe = frame->avFrame();
    auto size = QSize(avframe->width, avframe->height);
    if (d_ptr->frameConverterPtr.isNull()) {
        d_ptr->frameConverterPtr.reset(new VideoFrameConverter(frame.data(), size, AV_PIX_FMT_RGBA));
    } else {
        d_ptr->frameConverterPtr->flush(frame.data(), size, AV_PIX_FMT_RGBA);
    }
    QSharedPointer<Frame> frameRgbPtr(new Frame);
    frameRgbPtr->imageAlloc(size, AV_PIX_FMT_RGBA);
    d_ptr->frameConverterPtr->scaleToQImage(frame.data(), frameRgbPtr.data(), size);
    //    qDebug() << frameRgbPtr->avFrame()->width << frameRgbPtr->avFrame()->height
    //             << frameRgbPtr->avFrame()->format;
    return frameRgbPtr;
}

QVector<AVPixelFormat> OpenglRender::supportedOutput_pix_fmt()
{
    return d_ptr->supportFormats;
}

void OpenglRender::updateFrame(QSharedPointer<Frame> frame)
{
    QMetaObject::invokeMethod(
        this, [=] { displayFrame(frame); }, Qt::QueuedConnection);
}

void OpenglRender::initTexture()
{
    // 绑定YUV 变量值
    d_ptr->programPtr->setUniformValue("tex_y", 0);
    d_ptr->programPtr->setUniformValue("tex_u", 1);
    d_ptr->programPtr->setUniformValue("tex_v", 2);
    d_ptr->programPtr->setUniformValue("tex_uv", 3);
    d_ptr->programPtr->setUniformValue("tex_rgba", 4);

    glGenTextures(1, &d_ptr->textureY);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureY);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &d_ptr->textureU);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureU);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &d_ptr->textureV);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureV);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &d_ptr->textureUV);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureUV);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &d_ptr->textureRGBA);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureRGBA);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void OpenglRender::updateYUV420P()
{
    auto frame = d_ptr->framePtr->avFrame();
    // Y
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureY);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width,
                 frame->height,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[0]);
    d_ptr->programPtr->setUniformValue("tex_y", 0);
    // U
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureU);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width / 2,
                 frame->height / 2,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[1]);
    d_ptr->programPtr->setUniformValue("tex_u", 1);
    // V
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureV);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width / 2,
                 frame->height / 2,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[2]);
    d_ptr->programPtr->setUniformValue("tex_v", 2);
}

void OpenglRender::updateYUYV422()
{
    auto frame = d_ptr->framePtr->avFrame();
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureRGBA);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 frame->width / 2,
                 frame->height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 frame->data[0]);
    d_ptr->programPtr->setUniformValue("tex_rgba", 4);
}

void OpenglRender::updateYUV422P()
{
    auto frame = d_ptr->framePtr->avFrame();
    // Y
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureY);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width,
                 frame->height,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[0]);
    d_ptr->programPtr->setUniformValue("tex_y", 0);
    // U
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureU);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width / 2,
                 frame->height,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[1]);
    d_ptr->programPtr->setUniformValue("tex_u", 1);
    // V
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureV);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width / 2,
                 frame->height,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[2]);
    d_ptr->programPtr->setUniformValue("tex_v", 2);
}

void OpenglRender::updateYUV444P()
{
    auto frame = d_ptr->framePtr->avFrame();
    // Y
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureY);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width,
                 frame->height,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[0]);
    d_ptr->programPtr->setUniformValue("tex_y", 0);
    // U
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureU);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width,
                 frame->height,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[1]);
    d_ptr->programPtr->setUniformValue("tex_u", 1);
    // V
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureV);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width,
                 frame->height,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[2]);
    d_ptr->programPtr->setUniformValue("tex_v", 2);
}

void OpenglRender::updateNV12()
{
    auto frame = d_ptr->framePtr->avFrame();
    // Y
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureY);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width,
                 frame->height,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[0]);
    d_ptr->programPtr->setUniformValue("tex_y", 0);
    // UV
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureUV);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE_ALPHA, //GL_RG, GL_LUMINANCE_ALPHA
                 frame->width / 2,
                 frame->height / 2,
                 0,
                 GL_LUMINANCE_ALPHA, //GL_RG, GL_LUMINANCE_ALPHA
                 GL_UNSIGNED_BYTE,
                 frame->data[1]);
    d_ptr->programPtr->setUniformValue("tex_uv", 3);
}

void OpenglRender::updateRGB()
{
    auto frame = d_ptr->framePtr->avFrame();
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureRGBA);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 frame->width,
                 frame->height,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 frame->data[0]);
    d_ptr->programPtr->setUniformValue("tex_rgba", 4);
}

void OpenglRender::updateRGBA()
{
    auto frame = d_ptr->framePtr->avFrame();
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureRGBA);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 frame->width,
                 frame->height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 frame->data[0]);
    d_ptr->programPtr->setUniformValue("tex_rgba", 4);
}

void OpenglRender::displayFrame(QSharedPointer<Frame> framePtr)
{
    auto avFrame = framePtr->avFrame();
    // 如果帧长宽为0则不需要绘制
    if (avFrame->width <= 0 || avFrame->height <= 0) {
        return;
    }
    d_ptr->framePtr = framePtr;
    if (d_ptr->size.width() != avFrame->width || d_ptr->size.height() != avFrame->height) {
        d_ptr->size = QSize(avFrame->width, avFrame->height);
        resizeGL(width(), height());
    }

    update();
}

void OpenglRender::initializeGL()
{
    initializeOpenGLFunctions();

    // 加载shader脚本程序
    d_ptr->programPtr.reset(new QOpenGLShaderProgram(this));
    d_ptr->programPtr->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/shader.vert");
    d_ptr->programPtr->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/shader.frag");
    d_ptr->programPtr->link();
    d_ptr->programPtr->bind();

    initVbo();
    initTexture();
}

void OpenglRender::resizeGL(int w, int h)
{
    if (d_ptr->size.width() < 0 || d_ptr->size.height() < 0) {
        return;
    }

    auto size = d_ptr->size.scaled(w, h, Qt::KeepAspectRatio);
    d_ptr->frameRect = QRectF((w - size.width()) / 2,
                              (h - size.height()) / 2,
                              size.width(),
                              size.height());
}

void OpenglRender::paintGL()
{
    // 将窗口的位平面区域（背景）设置为先前由glClearColor、glClearDepth和选择的值
    glClearColor(d_ptr->backgroundColor.redF(),
                 d_ptr->backgroundColor.greenF(),
                 d_ptr->backgroundColor.blueF(),
                 d_ptr->backgroundColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (d_ptr->framePtr.isNull()) {
        return;
    }

    // 设置视图大小实现图片自适应
    glViewport(d_ptr->frameRect.x() * devicePixelRatio(),
               d_ptr->frameRect.y() * devicePixelRatio(),
               d_ptr->frameRect.width() * devicePixelRatio(),
               d_ptr->frameRect.height() * devicePixelRatio());
    auto format = d_ptr->framePtr->avFrame()->format;

    d_ptr->programPtr->bind(); // 绑定着色器
    d_ptr->programPtr->setUniformValue("format", format);
#ifndef QT_NO_DEBUG
    //qDebug() << format;
#endif
    // 绑定纹理
    switch (format) {
    case AV_PIX_FMT_YUV420P: updateYUV420P(); break;
    case AV_PIX_FMT_YUYV422: updateYUYV422(); break;
    case AV_PIX_FMT_RGB24:
    case AV_PIX_FMT_BGR24: updateRGB(); break;
    case AV_PIX_FMT_YUV422P: updateYUV422P(); break;
    case AV_PIX_FMT_YUV444P: updateYUV444P(); break;
    case AV_PIX_FMT_NV12: updateNV12(); break;
    case AV_PIX_FMT_ARGB:
    case AV_PIX_FMT_RGBA:
    case AV_PIX_FMT_ABGR:
    case AV_PIX_FMT_BGRA:
    case AV_PIX_FMT_0RGB:
    case AV_PIX_FMT_RGB0:
    case AV_PIX_FMT_0BGR:
    case AV_PIX_FMT_BGR0: updateRGBA(); break;
    default: break;
    }

    glBindVertexArray(d_ptr->vao); // 绑定VAO
    glDrawElements(
        GL_TRIANGLES,    // 绘制的图元类型
        6,               // 指定要渲染的元素数(点数)
        GL_UNSIGNED_INT, // 指定索引中值的类型(indices)
        nullptr); // 指定当前绑定到GL_ELEMENT_array_buffer目标的缓冲区的数据存储中数组中第一个索引的偏移量。
    glBindVertexArray(0);

    d_ptr->programPtr->release();
}

void OpenglRender::initVbo()
{
    // 返回属性名称在此着色器程序的参数列表中的位置。如果名称不是此着色器程序的有效属性，则返回-1。
    GLuint posAttr = GLuint(d_ptr->programPtr->attributeLocation("aPos"));
    GLuint texCord = GLuint(d_ptr->programPtr->attributeLocation("aTexCord"));

    glBindVertexArray(d_ptr->vao);

    float vertices[] = {
        // positions             // texture coords
        1.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top right
        1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom right
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
        -1.0f, 1.0f,  0.0f, 0.0f, 1.0f  // top left
    };
    unsigned int indices[] = {0, 1, 3, 1, 2, 3};

    d_ptr->vbo.destroy();
    d_ptr->vbo.create();
    d_ptr->vbo.bind();
    d_ptr->vbo.allocate(vertices, sizeof(vertices));

    d_ptr->ebo.destroy();
    d_ptr->ebo.create();
    d_ptr->ebo.bind();
    d_ptr->ebo.allocate(indices, sizeof(indices));

    d_ptr->programPtr->setAttributeBuffer(posAttr, GL_FLOAT, 0, 3, sizeof(float) * 5);
    d_ptr->programPtr->enableAttributeArray(0);
    d_ptr->programPtr->setAttributeBuffer(texCord, GL_FLOAT, 3 * sizeof(float), 2, sizeof(float) * 5);
    d_ptr->programPtr->enableAttributeArray(1);

    // 启用通用顶点属性数组
    glEnableVertexAttribArray(
        texCord); // 属性索引是从调用glGetAttribLocation接收的，或者传递给glBindAttribLocation。

    // 释放
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0); // 设置为零以破坏现有的顶点数组对象绑定}
}

} // namespace Ffmpeg
