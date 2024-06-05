#include "mpvopenglwidget.hpp"
#include "mpvplayer.hpp"

#include <QOpenGLContext>

#include <mpv/client.h>
#include <mpv/render_gl.h>

namespace Mpv {

static auto get_proc_address(void *ctx, const char *name) -> void *
{
    Q_UNUSED(ctx);
    auto *glctx = QOpenGLContext::currentContext();
    if (glctx == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<void *>(glctx->getProcAddress(QByteArray(name)));
}

class MpvOpenglWidget::MpvOpenglWidgetPrivate
{
public:
    MpvOpenglWidgetPrivate(MpvOpenglWidget *parent)
        : owner(parent)
    {
        resizeTimer = new QTimer(owner);
        resizeTimer->setSingleShot(true);
    }

    void clean()
    {
        if (mpv_gl) {
            mpv_render_context_free(mpv_gl);
        }
    }

    MpvOpenglWidget *owner;
    MpvPlayer *mpvPlayer = nullptr;
    mpv_render_context *mpv_gl;

    bool resizePauseState = false;
    bool isResizing = false;
    QTimer *resizeTimer;
};

MpvOpenglWidget::MpvOpenglWidget(MpvPlayer *mpvPlayer, QWidget *parent)
    : QOpenGLWidget{parent}
    , d_ptr(new MpvOpenglWidgetPrivate(this))
{
    d_ptr->mpvPlayer = mpvPlayer;

    buildConnect();
}

MpvOpenglWidget::~MpvOpenglWidget()
{
    makeCurrent();
    d_ptr->clean();
    d_ptr->mpvPlayer->quit();
    doneCurrent();
}

void MpvOpenglWidget::initializeGL()
{
    mpv_opengl_init_params gl_init_params{get_proc_address, nullptr};
    mpv_render_param params[]{{MPV_RENDER_PARAM_API_TYPE,
                               const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
                              {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
                              {MPV_RENDER_PARAM_INVALID, nullptr}};

    if (mpv_render_context_create(&d_ptr->mpv_gl, d_ptr->mpvPlayer->mpv_handler(), params) < 0)
        throw std::runtime_error("failed to initialize mpv GL context");
    mpv_render_context_set_update_callback(d_ptr->mpv_gl,
                                           MpvOpenglWidget::on_update,
                                           reinterpret_cast<void *>(this));
}

void MpvOpenglWidget::paintGL()
{
    auto devicePixelRatio = this->devicePixelRatio();
    mpv_opengl_fbo mpfbo{static_cast<int>(defaultFramebufferObject()),
                         int(width() * devicePixelRatio),
                         int(height() * devicePixelRatio),
                         0};
    int flip_y{1};

    mpv_render_param params[] = {{MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
                                 {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
                                 {MPV_RENDER_PARAM_INVALID, nullptr}};
    // See render_gl.h on what OpenGL environment mpv expects, and
    // other API details.
    mpv_render_context_render(d_ptr->mpv_gl, params);
}

void MpvOpenglWidget::maybeUpdate()
{
    // If the Qt window is not visible, Qt's update() will just skip rendering.
    // This confuses mpv's render API, and may lead to small occasional
    // freezes due to video rendering timing out.
    // Handle this by manually redrawing.
    // Note: Qt doesn't seem to provide a way to query whether update() will
    //       be skipped, and the following code still fails when e.g. switching
    //       to a different workspace with a reparenting window manager.
    if (window()->isMinimized()) {
        makeCurrent();
        paintGL();
        context()->swapBuffers(context()->surface());
        doneCurrent();
    } else {
        update();
    }
}

void MpvOpenglWidget::onBeforeResize()
{
    if (d_ptr->isResizing) { // MacOS 全屏切换时会触发两次 resize
        return;
    }
    d_ptr->resizePauseState = d_ptr->mpvPlayer->isPaused();
    if (d_ptr->resizePauseState) {
        return;
    }
    d_ptr->mpvPlayer->pauseSync(true);
    d_ptr->isResizing = true;
}

void MpvOpenglWidget::onAfterResize()
{
    d_ptr->isResizing = false;
    if (d_ptr->resizePauseState) {
        return;
    }
    d_ptr->mpvPlayer->pauseAsync();
}

void MpvOpenglWidget::on_update(void *ctx)
{
    QMetaObject::invokeMethod((MpvOpenglWidget *) ctx, "maybeUpdate");
}

void MpvOpenglWidget::buildConnect()
{
    connect(this, &QOpenGLWidget::aboutToResize, this, &MpvOpenglWidget::onBeforeResize);
    connect(
        this,
        &QOpenGLWidget::resized,
        d_ptr->resizeTimer,
        [this] { d_ptr->resizeTimer->start(500); },
        Qt::QueuedConnection);

    connect(d_ptr->resizeTimer, &QTimer::timeout, this, &MpvOpenglWidget::onAfterResize);
}

} // namespace Mpv
