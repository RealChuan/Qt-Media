#include "videooutputrenderopenglrender.hpp"

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

namespace Ffmpeg {

class VideoOutputRenderOpenGLRender::VideoOutputRenderOpenGLRenderPrivate
{
public:
    VideoOutputRenderOpenGLRenderPrivate(QWidget *parent)
        : owner(parent)
    {
        transform.setToIdentity();
    }

    QWidget *owner;
    QOpenGLBuffer vbo1 = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    QOpenGLBuffer vbo2 = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    QScopedPointer<QOpenGLShaderProgram> programPtr;
    unsigned int texture;
    QMatrix4x4 transform;
    QColor backgroundColor = Qt::black;
};

VideoOutputRenderOpenGLRender::VideoOutputRenderOpenGLRender(QWidget *parent)
    : QOpenGLWidget(parent)
    , d_ptr(new VideoOutputRenderOpenGLRenderPrivate(this))
{}

VideoOutputRenderOpenGLRender::~VideoOutputRenderOpenGLRender()
{
    makeCurrent();
    d_ptr->vbo1.destroy();
    d_ptr->vbo2.destroy();
    d_ptr->programPtr.reset();
    doneCurrent();
}

void VideoOutputRenderOpenGLRender::onReadyRead(const QImage &image)
{
    bool isFirst = m_image.isNull();
    if (image.isNull()) {
        qWarning() << "image is null!";
        return;
    }
    m_image = image;
    checkSubtitle();
    if (isFirst) {
        resizeGL(width(), height());
    }
    update();
}

void VideoOutputRenderOpenGLRender::onSubtitleImages(const QVector<SubtitleImage> &subtitleImages)
{
    m_subtitleImages = subtitleImages;
    checkSubtitle();
    update();
}

void VideoOutputRenderOpenGLRender::initializeGL()
{
    initializeOpenGLFunctions();

    initVbo();

    glEnable(GL_DEPTH_TEST);
    //    glEnable(GL_CULL_FACE);

    initShader();

    initTexture();
}

void VideoOutputRenderOpenGLRender::paintGL()
{
    glClearColor(d_ptr->backgroundColor.redF(),
                 d_ptr->backgroundColor.greenF(),
                 d_ptr->backgroundColor.blueF(),
                 d_ptr->backgroundColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_image.isNull()) {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_BGRA,
                     m_image.width(),
                     m_image.height(),
                     0,
                     GL_BGRA,
                     GL_UNSIGNED_BYTE,
                     m_image.constBits());
        glGenerateMipmap(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, d_ptr->texture);
    }
    d_ptr->programPtr->setUniformValue(d_ptr->programPtr->uniformLocation("transform"),
                                       d_ptr->transform);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void VideoOutputRenderOpenGLRender::resizeGL(int width, int height)
{
    int side = qMin(width, height);
    glViewport((width - side) / 2, (height - side) / 2, side, side);
    if (m_image.isNull()) {
        return;
    }
    d_ptr->transform.setToIdentity();
    float ws = (GLfloat) m_image.width() / (GLfloat) width;
    float hs = (GLfloat) m_image.height() / (GLfloat) height;
    qInfo() << ws << hs;
    float maxs = qMax(ws, hs);
    d_ptr->transform.scale(ws / maxs, hs / maxs, 1);
}

void VideoOutputRenderOpenGLRender::initVbo()
{
    float vertices[] = {
        // positions             // texture coords
        1.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top right
        1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom right
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
        -1.0f, 1.0f,  0.0f, 0.0f, 1.0f  // top left
    };
    unsigned int indices[] = {0, 1, 3, 1, 2, 3};

    d_ptr->vbo1.destroy();
    d_ptr->vbo1.create();
    d_ptr->vbo1.bind();
    d_ptr->vbo1.allocate(vertices, sizeof(vertices));

    d_ptr->vbo2.destroy();
    d_ptr->vbo2.create();
    d_ptr->vbo2.bind();
    d_ptr->vbo2.allocate(indices, sizeof(indices));
}

void VideoOutputRenderOpenGLRender::initShader()
{
    d_ptr->programPtr.reset(new QOpenGLShaderProgram);
    qInfo() << d_ptr->programPtr->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/video.vs");
    qInfo() << d_ptr->programPtr->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/video.fs");
    d_ptr->programPtr->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(float) * 5);
    d_ptr->programPtr->enableAttributeArray(0);
    d_ptr->programPtr->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 2, sizeof(float) * 5);
    d_ptr->programPtr->enableAttributeArray(1);

    d_ptr->programPtr->link();
    d_ptr->programPtr->bind();
}

void VideoOutputRenderOpenGLRender::initTexture()
{
    glGenTextures(1, &d_ptr->texture);
    glBindTexture(GL_TEXTURE_2D,
                  d_ptr->texture); // all upcoming GL_TEXTURE_2D operations now have
    // effect on this texture object
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_S,
                    GL_REPEAT); // set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

} // namespace Ffmpeg
