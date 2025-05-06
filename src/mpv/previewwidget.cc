#include "previewwidget.hpp"
#include "mpvopenglwidget.hpp"
#include "mpvplayer.hpp"
#include "mpvwidget.hpp"

#include <QtWidgets>

namespace Mpv {

class PreviewWidget::PreviewWidgetPrivate
{
public:
    explicit PreviewWidgetPrivate(PreviewWidget *q)
        : q_ptr(q)
    {
        mpvPlayer = new Mpv::MpvPlayer(q_ptr);
#ifdef Q_OS_WIN
        mpvWidget = new Mpv::MpvWidget(q_ptr);
        mpvPlayer->initMpv(mpvWidget);
#else
        mpvWidget = new Mpv::MpvOpenglWidget(mpvPlayer, q_ptr);
        mpvPlayer->initMpv(nullptr);
#endif
        mpvPlayer->setHwdec("auto-safe");
        mpvPlayer->setCache(false);
        mpvPlayer->pauseAsync();
    }

    PreviewWidget *q_ptr;

    Mpv::MpvPlayer *mpvPlayer;
    QWidget *mpvWidget;
};

PreviewWidget::PreviewWidget(QWidget *parent)
    : QWidget(parent)
    , d_ptr(new PreviewWidgetPrivate(this))
{
    setupUI();
}

PreviewWidget::~PreviewWidget()
{
    delete d_ptr->mpvWidget;
}

void PreviewWidget::startPreview(const QString &filepath, int timestamp)
{
    if (filepath != d_ptr->mpvPlayer->filepath()) {
        d_ptr->mpvPlayer->openMedia(filepath);
        d_ptr->mpvPlayer->setAid("no");
        d_ptr->mpvPlayer->setSid("no");
    }
    d_ptr->mpvPlayer->seek(timestamp);
}

void PreviewWidget::clearAllTask()
{
    d_ptr->mpvPlayer->abortAllAsyncCommands();
}

void PreviewWidget::setupUI()
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d_ptr->mpvWidget);
}

} // namespace Mpv
