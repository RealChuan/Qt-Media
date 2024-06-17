#include "previewwidget.hpp"
#include "mpvopenglwidget.hpp"
#include "mpvplayer.hpp"
#include "mpvwidget.hpp"

#include <QtWidgets>

namespace Mpv {

class PreviewWidget::PreviewWidgetPrivate
{
public:
    PreviewWidgetPrivate(PreviewWidget *parent)
        : owner(parent)
    {
        mpvPlayer = new Mpv::MpvPlayer(owner);
#ifdef Q_OS_WIN
        mpvWidget = new Mpv::MpvWidget(owner);
        mpvPlayer->initMpv(mpvWidget);
#else
        mpvWidget = new Mpv::MpvOpenglWidget(mpvPlayer, owner);
        mpvPlayer->initMpv(nullptr);
#endif
        mpvPlayer->setHwdec("auto-safe");
        mpvPlayer->setCache(false);
        mpvPlayer->pauseAsync();
    }

    PreviewWidget *owner;

    Mpv::MpvPlayer *mpvPlayer;
    QWidget *mpvWidget;
};

PreviewWidget::PreviewWidget(QWidget *parent)
    : QWidget(parent)
    , d_ptr(new PreviewWidgetPrivate(this))
{
    setupUI();
}

PreviewWidget::~PreviewWidget() {}

void PreviewWidget::startPreview(const QString &filepath, int timestamp)
{
    if (filepath != d_ptr->mpvPlayer->filepath()) {
        d_ptr->mpvPlayer->openMedia(filepath);
        d_ptr->mpvPlayer->blockAudioTrack();
        d_ptr->mpvPlayer->blockSubTrack();
    }
    d_ptr->mpvPlayer->seek(timestamp);
}

void PreviewWidget::clearAllTask()
{
    d_ptr->mpvPlayer->abortAllAsyncCommands();
}

void PreviewWidget::setupUI()
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(QMargins());
    layout->setSpacing(0);
    layout->addWidget(d_ptr->mpvWidget);
}

} // namespace Mpv
