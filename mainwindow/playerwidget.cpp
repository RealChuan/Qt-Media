#include "playerwidget.h"

#include <utils/utils.h>

#include <QtWidgets>

class PlayerWidgetPrivate{
public:
    PlayerWidgetPrivate(QWidget *parent)
        : owner(parent){
        menu = new QMenu(owner);
    }
    QWidget *owner;
    QMenu *menu;
};

PlayerWidget::PlayerWidget(QWidget *parent)
    : VideoOutputWidget(parent)
    , d_ptr(new PlayerWidgetPrivate(this))
{
    setAcceptDrops(true);
    setupUI();
}

PlayerWidget::~PlayerWidget()
{

}

void PlayerWidget::dragEnterEvent(QDragEnterEvent *event)
{
    QWidget::dragEnterEvent(event);
    event->acceptProposedAction();
}

void PlayerWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QWidget::dragMoveEvent(event);
    event->acceptProposedAction();
}

void PlayerWidget::dropEvent(QDropEvent *event)
{
    QWidget::dropEvent(event);
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty())
        return;

    emit openFile(urls.first().toLocalFile());
}

void PlayerWidget::setupUI()
{
    d_ptr->menu->addAction(tr("Open Video"), this, &PlayerWidget::onOpenVideo);
}

void PlayerWidget::onOpenVideo()
{
    QString path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).value(0, QDir::homePath());
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),path,
                                                    tr("Audio Video (*.mp3 *.mp4 *.mkv *.rmvb)"));
    if(fileName.isEmpty())
        return;

    emit openFile(fileName);
}

void PlayerWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QWidget::contextMenuEvent(event);
    d_ptr->menu->exec(event->globalPos());
}
