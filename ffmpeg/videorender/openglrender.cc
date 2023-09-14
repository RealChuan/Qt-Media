#include "openglrender.hpp"
#include "openglshaderprogram.hpp"

#include <ffmpeg/colorspace.hpp>
#include <ffmpeg/frame.hpp>
#include <ffmpeg/subtitle.h>
#include <ffmpeg/videoframeconverter.hpp>

#include <QImage>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

class OpenglRender::OpenglRenderPrivate
{
public:
    explicit OpenglRenderPrivate(OpenglRender *q)
        : q_ptr(q)
    {}
    ~OpenglRenderPrivate() = default;

    OpenglRender *q_ptr;

    GLuint vao = 0; // 顶点数组对象,任何随后的顶点属性调用都会储存在这个VAO中，一个VAO可以有多个VBO

    QScopedPointer<OpenGLShaderProgram> programPtr;
    GLuint textureY;
    GLuint textureU;
    GLuint textureV;
    GLuint textureUV;
    GLuint textureRGBA;
    // sub
    QScopedPointer<OpenGLShaderProgram> subProgramPtr;
    GLuint textureSub;
    bool subChanged = false;

    const QVector<AVPixelFormat> supportFormats
        = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUYV422,     AV_PIX_FMT_RGB24,   AV_PIX_FMT_BGR24,
           AV_PIX_FMT_YUV422P, AV_PIX_FMT_YUV444P,     AV_PIX_FMT_YUV410P, AV_PIX_FMT_YUV411P,
           AV_PIX_FMT_UYVY422, AV_PIX_FMT_BGR8,        AV_PIX_FMT_RGB8,    AV_PIX_FMT_NV12,
           AV_PIX_FMT_NV21,    AV_PIX_FMT_ARGB,        AV_PIX_FMT_RGBA,    AV_PIX_FMT_ABGR,
           AV_PIX_FMT_BGRA,    AV_PIX_FMT_YUV420P10LE, AV_PIX_FMT_0RGB,    AV_PIX_FMT_RGB0,
           AV_PIX_FMT_0BGR,    AV_PIX_FMT_BGR0,        AV_PIX_FMT_P010LE};
    QScopedPointer<VideoFrameConverter> frameConverterPtr;

    QSharedPointer<Frame> framePtr;
    QSharedPointer<Subtitle> subTitleFramePtr;

    QColor backgroundColor = Qt::black;
};

OpenglRender::OpenglRender(QWidget *parent)
    : QOpenGLWidget(parent)
    , d_ptr(new OpenglRenderPrivate(this))
{
    auto format = this->format();
    qInfo() << "OpenGL Version:" << format.minorVersion() << "~" << format.majorVersion();
}

OpenglRender::~OpenglRender()
{
    if (isValid()) {
        return;
    }
    makeCurrent();
    glDeleteVertexArrays(1, &d_ptr->vao);
    d_ptr->programPtr.reset();
    d_ptr->subProgramPtr.reset();
    glDeleteTextures(1, &d_ptr->textureY);
    glDeleteTextures(1, &d_ptr->textureU);
    glDeleteTextures(1, &d_ptr->textureV);
    glDeleteTextures(1, &d_ptr->textureUV);
    glDeleteTextures(1, &d_ptr->textureRGBA);
    glDeleteTextures(1, &d_ptr->textureSub);
    doneCurrent();
}

auto OpenglRender::isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) -> bool
{
    return d_ptr->supportFormats.contains(pix_fmt);
}

auto OpenglRender::convertSupported_pix_fmt(QSharedPointer<Frame> frame) -> QSharedPointer<Frame>
{
    auto dst_pix_fmt = AV_PIX_FMT_RGBA;
    auto avframe = frame->avFrame();
    auto size = QSize(avframe->width, avframe->height);
    size.scale(this->size() * devicePixelRatio(), Qt::KeepAspectRatio);
    if (d_ptr->frameConverterPtr.isNull()) {
        d_ptr->frameConverterPtr.reset(new VideoFrameConverter(frame.data(), size, dst_pix_fmt));
    } else {
        d_ptr->frameConverterPtr->flush(frame.data(), size, dst_pix_fmt);
    }
    QSharedPointer<Frame> frameRgbPtr(new Frame);
    auto ret = frameRgbPtr->imageAlloc(size, dst_pix_fmt);
    if (!ret) {
        qWarning() << "imageAlloc failed";
        return QSharedPointer<Frame>();
    }
    d_ptr->frameConverterPtr->scale(frame.data(), frameRgbPtr.data());
    //    qDebug() << frameRgbPtr->avFrame()->width << frameRgbPtr->avFrame()->height
    //             << frameRgbPtr->avFrame()->format;
    return frameRgbPtr;
}

QVector<AVPixelFormat> OpenglRender::supportedOutput_pix_fmt()
{
    return d_ptr->supportFormats;
}

void OpenglRender::resetAllFrame()
{
    d_ptr->framePtr.reset();
    d_ptr->subTitleFramePtr.reset();
}

auto OpenglRender::widget() -> QWidget *
{
    return this;
}

void OpenglRender::updateFrame(QSharedPointer<Frame> frame)
{
    QMetaObject::invokeMethod(
        this,
        [=] {
            d_ptr->framePtr = frame;
            update();
        },
        Qt::QueuedConnection);
}

void OpenglRender::updateSubTitleFrame(QSharedPointer<Subtitle> frame)
{
    QMetaObject::invokeMethod(
        this,
        [=] {
            if (d_ptr->subTitleFramePtr.isNull()
                || d_ptr->subTitleFramePtr->image().size() != frame->image().size()) {
                d_ptr->subChanged = true;
            }
            d_ptr->subTitleFramePtr = frame;
            // need update?
            //update();
        },
        Qt::QueuedConnection);
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

void OpenglRender::initSubTexture()
{
    d_ptr->subProgramPtr->setUniformValue("tex", 0);
    glGenTextures(1, &d_ptr->textureSub);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureSub);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void OpenglRender::setColorSpace()
{
    auto avFrame = d_ptr->framePtr->avFrame();
    switch (avFrame->colorspace) {
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
        d_ptr->programPtr->setUniformValue("offset", ColorSpace::kBT601Offset);
        d_ptr->programPtr->setUniformValue("colorConversion", QMatrix3x3(ColorSpace::kBT601Matrix));
        break;
    case AVCOL_SPC_BT2020_NCL:
    case AVCOL_SPC_BT2020_CL:
        d_ptr->programPtr->setUniformValue("offset", ColorSpace::kBT2020ffset);
        d_ptr->programPtr->setUniformValue("colorConversion", QMatrix3x3(ColorSpace::kBT2020Matrix));
        break;
    //case AVCOL_SPC_BT709:
    default:
        d_ptr->programPtr->setUniformValue("offset", ColorSpace::kBT7090ffset);
        d_ptr->programPtr->setUniformValue("colorConversion", QMatrix3x3(ColorSpace::kBT709Matrix));
        break;
    }
}

auto OpenglRender::fitToScreen(const QSize &size) -> QMatrix4x4
{
    auto factor_w = qreal(width()) / size.width();
    auto factor_h = qreal(height()) / size.height();
    auto factor = qMin(factor_w, factor_h);
    QMatrix4x4 matrix;
    matrix.setToIdentity();
    matrix.scale(factor / factor_w, factor / factor_h);
    return matrix;
}

void OpenglRender::paintVideoFrame()
{
    auto avFrame = d_ptr->framePtr->avFrame();
    auto format = avFrame->format;
    //qDebug() << format;
    // 绑定纹理
    switch (format) {
    case AV_PIX_FMT_YUV420P: updateYUV420P(); break;
    case AV_PIX_FMT_YUYV422: updateYUYV422(); break;
    case AV_PIX_FMT_RGB24:
    case AV_PIX_FMT_BGR24: updateRGB(); break;
    case AV_PIX_FMT_YUV422P: updateYUV422P(); break;
    case AV_PIX_FMT_YUV444P: updateYUV444P(); break;
    case AV_PIX_FMT_YUV410P: updateYUV410P(); break;
    case AV_PIX_FMT_YUV411P: updateYUV411P(); break;
    case AV_PIX_FMT_UYVY422: updateUYVY422(); break;
    case AV_PIX_FMT_BGR8: updateRGB8(GL_UNSIGNED_BYTE_2_3_3_REV); break;
    case AV_PIX_FMT_RGB8: updateRGB8(GL_UNSIGNED_BYTE_3_3_2); break;
    case AV_PIX_FMT_NV12:
    case AV_PIX_FMT_NV21: updateNV12(); break;
    case AV_PIX_FMT_ARGB:
    case AV_PIX_FMT_RGBA:
    case AV_PIX_FMT_ABGR:
    case AV_PIX_FMT_BGRA:
    case AV_PIX_FMT_0RGB:
    case AV_PIX_FMT_RGB0:
    case AV_PIX_FMT_0BGR:
    case AV_PIX_FMT_BGR0: updateRGBA(); break;
    case AV_PIX_FMT_YUV420P10LE: updateYUV420P10LE(); break;
    case AV_PIX_FMT_P010LE: updateP010LE(); break;
    default: break;
    }
    d_ptr->programPtr->bind(); // 绑定着色器
    d_ptr->programPtr->setUniformValue("format", format);
    d_ptr->programPtr->setUniformValue("transform", fitToScreen({avFrame->width, avFrame->height}));
    setColorSpace();
    draw();
    d_ptr->programPtr->release();
}

void OpenglRender::paintSubTitleFrame()
{
    if (d_ptr->subTitleFramePtr.isNull() || d_ptr->framePtr.isNull()) {
        return;
    }
    if (d_ptr->subTitleFramePtr->pts() > d_ptr->framePtr->pts()
        || (d_ptr->subTitleFramePtr->pts() + d_ptr->subTitleFramePtr->duration())
               < d_ptr->framePtr->pts()) {
        return;
    }
    auto img = d_ptr->subTitleFramePtr->image();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureSub);
    if (d_ptr->subChanged) {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     img.width(),
                     img.height(),
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     img.constBits());
        d_ptr->subChanged = false;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        img.width(),
                        img.height(),
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        img.constBits());
    }

    glEnable(GL_BLEND);
    d_ptr->subProgramPtr->bind();
    d_ptr->subProgramPtr->setUniformValue("transform", fitToScreen(img.size()));
    draw();
    d_ptr->subProgramPtr->release();
    glDisable(GL_BLEND);
}

void OpenglRender::clear()
{
    // 将窗口的位平面区域（背景）设置为先前由glClearColor、glClearDepth和选择的值
    glClearColor(d_ptr->backgroundColor.redF(),
                 d_ptr->backgroundColor.greenF(),
                 d_ptr->backgroundColor.blueF(),
                 d_ptr->backgroundColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenglRender::draw()
{
    glBindVertexArray(d_ptr->vao); // 绑定VAO
    glDrawElements(
        GL_TRIANGLES,    // 绘制的图元类型
        6,               // 指定要渲染的元素数(点数)
        GL_UNSIGNED_INT, // 指定索引中值的类型(indices)
        nullptr); // 指定当前绑定到GL_ELEMENT_array_buffer目标的缓冲区的数据存储中数组中第一个索引的偏移量。
    glBindVertexArray(0);
}

void OpenglRender::initializeGL()
{
    initializeOpenGLFunctions();

    //glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_DITHER);
    clear();

    glGenVertexArrays(1, &d_ptr->vao);
    glBindVertexArray(d_ptr->vao);

    // 加载shader脚本程序
    d_ptr->programPtr.reset(new OpenGLShaderProgram(this));
    d_ptr->programPtr->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/video.vert");
    d_ptr->programPtr->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/video.frag");
    d_ptr->programPtr->link();
    d_ptr->programPtr->bind();
    d_ptr->programPtr->initVertex("aPos", "aTexCord");
    initTexture();
    d_ptr->programPtr->release();

    d_ptr->subProgramPtr.reset(new OpenGLShaderProgram(this));
    d_ptr->subProgramPtr->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/video.vert");
    d_ptr->subProgramPtr->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/sub.frag");
    d_ptr->subProgramPtr->link();
    d_ptr->subProgramPtr->bind();
    d_ptr->subProgramPtr->initVertex("aPos", "aTexCord");
    initSubTexture();
    d_ptr->subProgramPtr->release();

    // 释放
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0); // 设置为零以破坏现有的顶点数组对象绑定}
}

void OpenglRender::resizeGL(int w, int h)
{
    auto ratioF = devicePixelRatioF();
    glViewport(0, 0, w * ratioF, w * ratioF);
}

void OpenglRender::paintGL()
{
    clear();

    if (d_ptr->framePtr.isNull()) {
        return;
    }

    paintVideoFrame();
    paintSubTitleFrame();
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
}

void OpenglRender::updateYUV410P()
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
    // U
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureU);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width / 4,
                 frame->height / 4,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[1]);
    // V
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureV);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width / 4,
                 frame->height / 4,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[2]);
}

void OpenglRender::updateYUV411P()
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
    // U
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureU);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width / 4,
                 frame->height,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[1]);
    // V
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureV);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width / 4,
                 frame->height,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[2]);
}

void OpenglRender::updateUYVY422()
{
    auto frame = d_ptr->framePtr->avFrame();
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureRGBA);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA8,
                 frame->width / 2,
                 frame->height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 frame->data[0]);
}

void OpenglRender::updateRGB8(int dataType)
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
                 dataType,
                 frame->data[0]);
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
    // UV
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureUV);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RG, // GL_RG GL_LUMINANCE_ALPHA
                 frame->width / 2,
                 frame->height / 2,
                 0,
                 GL_RG, // GL_RG GL_LUMINANCE_ALPHA
                 GL_UNSIGNED_BYTE,
                 frame->data[1]);
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
}

void OpenglRender::updateYUV420P10LE()
{
    auto frame = d_ptr->framePtr->avFrame();
    // Y
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureY);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE_ALPHA,
                 frame->width,
                 frame->height,
                 0,
                 GL_LUMINANCE_ALPHA,
                 GL_UNSIGNED_BYTE,
                 frame->data[0]);
    // U
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureU);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE_ALPHA,
                 frame->width / 2,
                 frame->height / 2,
                 0,
                 GL_LUMINANCE_ALPHA,
                 GL_UNSIGNED_BYTE,
                 frame->data[1]);
    // V
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureV);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE_ALPHA,
                 frame->width / 2,
                 frame->height / 2,
                 0,
                 GL_LUMINANCE_ALPHA,
                 GL_UNSIGNED_BYTE,
                 frame->data[2]);
}

void OpenglRender::updateP010LE()
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
                 GL_UNSIGNED_SHORT,
                 frame->data[0]);
    // UV
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureUV);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE_ALPHA,
                 frame->width / 2,
                 frame->height / 2,
                 0,
                 GL_LUMINANCE_ALPHA,
                 GL_UNSIGNED_SHORT,
                 frame->data[1]);
}

} // namespace Ffmpeg
