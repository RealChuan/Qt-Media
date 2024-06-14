#include "openglrender.hpp"
#include "openglshader.hpp"
#include "openglshaderprogram.hpp"

#include <ffmpeg/colorutils.hpp>
#include <ffmpeg/frame.hpp>
#include <ffmpeg/subtitle.h>
#include <ffmpeg/videoframeconverter.hpp>
#include <mediaconfig/equalizer.hpp>
#include <utils/utils.h>

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
    // sub
    QScopedPointer<OpenGLShaderProgram> subProgramPtr;
    GLuint textureSub;

    const QVector<AVPixelFormat> supportFormats = {AV_PIX_FMT_YUV420P,
                                                   AV_PIX_FMT_YUYV422,
                                                   AV_PIX_FMT_RGB24,
                                                   AV_PIX_FMT_BGR24,
                                                   AV_PIX_FMT_YUV422P,
                                                   AV_PIX_FMT_YUV444P,
                                                   AV_PIX_FMT_YUV410P,
                                                   AV_PIX_FMT_YUV411P,
                                                   AV_PIX_FMT_UYVY422,
                                                   AV_PIX_FMT_BGR8,
                                                   AV_PIX_FMT_RGB8,
                                                   AV_PIX_FMT_NV12,
                                                   AV_PIX_FMT_NV21,
                                                   AV_PIX_FMT_ARGB,
                                                   AV_PIX_FMT_RGBA,
                                                   AV_PIX_FMT_ABGR,
                                                   AV_PIX_FMT_BGRA,
                                                   AV_PIX_FMT_P010LE};
    QScopedPointer<VideoFrameConverter> frameConverterPtr;

    QSharedPointer<Frame> framePtr;
    bool frameChanged = true;
    QSharedPointer<Subtitle> subTitleFramePtr;
    bool subChanged = true;
    Tonemap::Type tonemapType;
    ColorUtils::Primaries::Type destPrimaries;
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
    cleanup();
    d_ptr->subProgramPtr.reset();
    glDeleteTextures(1, &d_ptr->textureSub);
    doneCurrent();
}

auto OpenglRender::isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) -> bool
{
    return d_ptr->supportFormats.contains(pix_fmt);
}

auto OpenglRender::convertSupported_pix_fmt(const QSharedPointer<Frame> &frame)
    -> QSharedPointer<Frame>
{
    auto dst_pix_fmt = AV_PIX_FMT_RGBA;
    auto *avframe = frame->avFrame();
    auto size = QSize(avframe->width, avframe->height);
    // 部分图像格式转换存在问题，比如转换成BGR8格式，会导致图像错位
    // size.scale(this->size() * devicePixelRatio(), Qt::KeepAspectRatio);
    if (d_ptr->frameConverterPtr.isNull()) {
        d_ptr->frameConverterPtr.reset(new VideoFrameConverter(frame.data(), size, dst_pix_fmt));
    } else {
        d_ptr->frameConverterPtr->flush(frame.data(), size, dst_pix_fmt);
    }
    QSharedPointer<Frame> frameRgbPtr(new Frame);
    auto ret = frameRgbPtr->imageAlloc(size, dst_pix_fmt);
    if (!ret) {
        qWarning() << "imageAlloc failed";
        return {};
    }
    d_ptr->frameConverterPtr->scale(frame.data(), frameRgbPtr.data());
    //    qDebug() << frameRgbPtr->avFrame()->width << frameRgbPtr->avFrame()->height
    //             << frameRgbPtr->avFrame()->format;
    return frameRgbPtr;
}

auto OpenglRender::supportedOutput_pix_fmt() -> QVector<AVPixelFormat>
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

void OpenglRender::updateFrame(const QSharedPointer<Frame> &framePtr)
{
    QMetaObject::invokeMethod(
        this, [=] { onUpdateFrame(framePtr); }, Qt::QueuedConnection);
}

void OpenglRender::updateSubTitleFrame(const QSharedPointer<Subtitle> &framePtr)
{
    QMetaObject::invokeMethod(
        this, [=] { onUpdateSubTitleFrame(framePtr); }, Qt::QueuedConnection);
}

void OpenglRender::initTexture()
{
    // 绑定YUV 变量值
    d_ptr->programPtr->setUniformValue("tex_y", 0);
    d_ptr->programPtr->setUniformValue("tex_u", 1);
    d_ptr->programPtr->setUniformValue("tex_v", 2);
    d_ptr->programPtr->setUniformValue("tex_rgba", 3);

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

auto OpenglRender::fitToScreen(const QSize &size) -> QMatrix4x4
{
    auto factor_w = static_cast<qreal>(width()) / size.width();
    auto factor_h = static_cast<qreal>(height()) / size.height();
    auto factor = qMin(factor_w, factor_h);
    QMatrix4x4 matrix;
    matrix.setToIdentity();
    matrix.scale(factor / factor_w, factor / factor_h);
    return matrix;
}

void OpenglRender::cleanup()
{
    d_ptr->programPtr.reset(new OpenGLShaderProgram(this));
    if (d_ptr->textureY > 0) {
        glDeleteTextures(1, &d_ptr->textureY);
    }
    if (d_ptr->textureU > 0) {
        glDeleteTextures(1, &d_ptr->textureU);
    }
    if (d_ptr->textureV > 0) {
        glDeleteTextures(1, &d_ptr->textureV);
    }
}

void OpenglRender::resetShader(Frame *frame)
{
    makeCurrent();
    cleanup();
    d_ptr->programPtr->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/video.vert");
    OpenglShader shader;
    d_ptr->programPtr->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                               shader.generate(frame,
                                                               m_tonemapType,
                                                               m_destPrimaries));
    glBindVertexArray(d_ptr->vao);
    d_ptr->programPtr->link();
    d_ptr->programPtr->bind();
    d_ptr->programPtr->initVertex("aPos", "aTexCord");
    initTexture();
    if (shader.isConvertPrimaries()) {
        d_ptr->programPtr->setUniformValue("cms_matrix", shader.convertPrimariesMatrix());
        qDebug() << "CMS matrix:" << shader.convertPrimariesMatrix();
    }
    auto param = Ffmpeg::ColorUtils::getYuvToRgbParam(frame);
    d_ptr->programPtr->setUniformValue("offset", param.offset);
    d_ptr->programPtr->setUniformValue("colorConversion", param.matrix);
    d_ptr->programPtr->release();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    doneCurrent();
}

void OpenglRender::onUpdateFrame(const QSharedPointer<Frame> &framePtr)
{
    if (d_ptr->framePtr.isNull()
        || d_ptr->framePtr->avFrame()->format != framePtr->avFrame()->format
        || m_tonemapType != d_ptr->tonemapType || m_destPrimaries != d_ptr->destPrimaries) {
        resetShader(framePtr.data());
        d_ptr->frameChanged = true;
        d_ptr->tonemapType = m_tonemapType;
        d_ptr->destPrimaries = m_destPrimaries;
    } else if (d_ptr->framePtr->avFrame()->width != framePtr->avFrame()->width
               || d_ptr->framePtr->avFrame()->height != framePtr->avFrame()->height) {
        d_ptr->frameChanged = true;
    }
    d_ptr->framePtr = framePtr;
    update();
}

void OpenglRender::onUpdateSubTitleFrame(const QSharedPointer<Subtitle> &framePtr)
{
    if (d_ptr->subTitleFramePtr.isNull()
        || d_ptr->subTitleFramePtr->image().size() != framePtr->image().size()) {
        d_ptr->subChanged = true;
    }
    d_ptr->subTitleFramePtr = framePtr;
    // need update?
    //update();
}

void OpenglRender::paintVideoFrame()
{
    auto *avFrame = d_ptr->framePtr->avFrame();
    auto format = avFrame->format;
    // qDebug() << format;
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
    case AV_PIX_FMT_NV21: updateNV12(GL_UNSIGNED_BYTE); break;
    case AV_PIX_FMT_ARGB:
    case AV_PIX_FMT_RGBA:
    case AV_PIX_FMT_ABGR:
    case AV_PIX_FMT_BGRA: updateRGBA(); break;
    case AV_PIX_FMT_P010LE: updateNV12(GL_UNSIGNED_SHORT); break;
    default: break;
    }
    d_ptr->programPtr->bind(); // 绑定着色器
    d_ptr->programPtr->setUniformValue("transform", fitToScreen({avFrame->width, avFrame->height}));
    d_ptr->programPtr->setUniformValue("contrast", m_equalizer.ffContrast());
    d_ptr->programPtr->setUniformValue("saturation", m_equalizer.ffSaturation());
    d_ptr->programPtr->setUniformValue("brightness", m_equalizer.ffBrightness());
    d_ptr->programPtr->setUniformValue("gamma", m_equalizer.ffGamma());
    d_ptr->programPtr->setUniformValue("hue", m_equalizer.ffHue());
    draw();
    d_ptr->programPtr->release();
    d_ptr->frameChanged = false;
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
    glClearColor(m_backgroundColor.redF(),
                 m_backgroundColor.greenF(),
                 m_backgroundColor.blueF(),
                 m_backgroundColor.alphaF());
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
    auto *frame = d_ptr->framePtr->avFrame();
    std::array<GLuint, 3> texs = {d_ptr->textureY, d_ptr->textureU, d_ptr->textureV};
    for (int i = 0; i < texs.size(); i++) {
        auto width = frame->width;
        auto height = frame->height;
        if (i > 0) {
            width /= 2;
            height /= 2;
        }
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, texs[i]);
        if (d_ptr->frameChanged) {
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RED,
                         width,
                         height,
                         0,
                         GL_RED,
                         GL_UNSIGNED_BYTE,
                         frame->data[i]);
        } else {
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            0,
                            0,
                            width,
                            height,
                            GL_RED,
                            GL_UNSIGNED_BYTE,
                            frame->data[i]);
        }
    }
}

void OpenglRender::updateYUYV422()
{
    auto *frame = d_ptr->framePtr->avFrame();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureY);
    if (d_ptr->frameChanged) {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     frame->width / 2,
                     frame->height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     frame->data[0]);
    } else {
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        frame->width / 2,
                        frame->height,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        frame->data[0]);
    }
}

void OpenglRender::updateYUV422P()
{
    auto *frame = d_ptr->framePtr->avFrame();
    std::array<GLuint, 3> texs = {d_ptr->textureY, d_ptr->textureU, d_ptr->textureV};
    for (int i = 0; i < texs.size(); i++) {
        auto width = frame->width;
        auto height = frame->height;
        if (i > 0) {
            width /= 2;
        }
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, texs[i]);
        if (d_ptr->frameChanged) {
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RED,
                         width,
                         height,
                         0,
                         GL_RED,
                         GL_UNSIGNED_BYTE,
                         frame->data[i]);
        } else {
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            0,
                            0,
                            width,
                            height,
                            GL_RED,
                            GL_UNSIGNED_BYTE,
                            frame->data[i]);
        }
    }
}

void OpenglRender::updateYUV444P()
{
    auto *frame = d_ptr->framePtr->avFrame();
    std::array<GLuint, 3> texs = {d_ptr->textureY, d_ptr->textureU, d_ptr->textureV};
    for (int i = 0; i < texs.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, texs[i]);
        if (d_ptr->frameChanged) {
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RED,
                         frame->width,
                         frame->height,
                         0,
                         GL_RED,
                         GL_UNSIGNED_BYTE,
                         frame->data[i]);
        } else {
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            0,
                            0,
                            frame->width,
                            frame->height,
                            GL_RED,
                            GL_UNSIGNED_BYTE,
                            frame->data[i]);
        }
    }
}

void OpenglRender::updateYUV410P()
{
    auto *frame = d_ptr->framePtr->avFrame();
    std::array<GLuint, 3> texs = {d_ptr->textureY, d_ptr->textureU, d_ptr->textureV};
    for (int i = 0; i < texs.size(); i++) {
        auto width = frame->width;
        auto height = frame->height;
        if (i > 0) {
            width /= 4;
            height /= 4;
        }
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, texs[i]);
        if (d_ptr->frameChanged) {
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RED,
                         width,
                         height,
                         0,
                         GL_RED,
                         GL_UNSIGNED_BYTE,
                         frame->data[i]);
        } else {
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            0,
                            0,
                            width,
                            height,
                            GL_RED,
                            GL_UNSIGNED_BYTE,
                            frame->data[i]);
        }
    }
}

void OpenglRender::updateYUV411P()
{
    auto *frame = d_ptr->framePtr->avFrame();
    std::array<GLuint, 3> texs = {d_ptr->textureY, d_ptr->textureU, d_ptr->textureV};
    for (int i = 0; i < texs.size(); i++) {
        auto width = frame->width;
        auto height = frame->height;
        if (i > 0) {
            width /= 4;
        }
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, texs[i]);
        if (d_ptr->frameChanged) {
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RED,
                         width,
                         height,
                         0,
                         GL_RED,
                         GL_UNSIGNED_BYTE,
                         frame->data[i]);
        } else {
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            0,
                            0,
                            width,
                            height,
                            GL_RED,
                            GL_UNSIGNED_BYTE,
                            frame->data[i]);
        }
    }
}

void OpenglRender::updateUYVY422()
{
    auto *frame = d_ptr->framePtr->avFrame();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureY);
    if (d_ptr->frameChanged) {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA8,
                     frame->width / 2,
                     frame->height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     frame->data[0]);
    } else {
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        frame->width / 2,
                        frame->height,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        frame->data[0]);
    }
}

void OpenglRender::updateRGB8(int dataType)
{
    auto *frame = d_ptr->framePtr->avFrame();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureY);
    if (d_ptr->frameChanged) {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGB,
                     frame->width,
                     frame->height,
                     0,
                     GL_RGB,
                     dataType,
                     frame->data[0]);
    } else {
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        frame->width,
                        frame->height,
                        GL_RGB,
                        dataType,
                        frame->data[0]);
    }
}

void OpenglRender::updateNV12(GLenum type)
{
    auto *frame = d_ptr->framePtr->avFrame();
    std::array<GLuint, 2> texs = {d_ptr->textureY, d_ptr->textureU};
    for (int i = 0; i < texs.size(); i++) {
        auto width = frame->width;
        auto height = frame->height;
        GLint format = GL_RED;
        if (i > 0) {
            width /= 2;
            height /= 2;
            format = GL_RG;
        }
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, texs[i]);
        if (d_ptr->frameChanged) {
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, type, frame->data[i]);
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, type, frame->data[i]);
        }
    }
}

void OpenglRender::updateRGB()
{
    auto *frame = d_ptr->framePtr->avFrame();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureY);
    if (d_ptr->frameChanged) {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGB,
                     frame->width,
                     frame->height,
                     0,
                     GL_RGB,
                     GL_UNSIGNED_BYTE,
                     frame->data[0]);
    } else {
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        frame->width,
                        frame->height,
                        GL_RGB,
                        GL_UNSIGNED_BYTE,
                        frame->data[0]);
    }
}

void OpenglRender::updateRGBA()
{
    auto *frame = d_ptr->framePtr->avFrame();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d_ptr->textureY);
    if (d_ptr->frameChanged) {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     frame->width,
                     frame->height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     frame->data[0]);
    } else {
        glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        frame->width,
                        frame->height,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        frame->data[0]);
    }
}

} // namespace Ffmpeg
