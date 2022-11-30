#include "playerwidget.h"

#include <utils/utils.h>

#include <QtWidgets>

class PlayerWidget::PlayerWidgetPrivate
{
public:
    PlayerWidgetPrivate(QWidget *parent)
        : owner(parent)
    {
        menu = new QMenu(owner);
    }
    QWidget *owner;

    QMenu *menu;
};

PlayerWidget::PlayerWidget(QWidget *parent)
    : Ffmpeg::OpenglRender(parent)
    , d_ptr(new PlayerWidgetPrivate(this))
{
    setAcceptDrops(true);
    setupUI();
}

PlayerWidget::~PlayerWidget() {}

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
    d_ptr->menu->addAction(tr("Open Local Media"), this, &PlayerWidget::onOpenLocalMedia);
    d_ptr->menu->addAction(tr("Open Web Media"), this, &PlayerWidget::onOpenWebMedia);
}

void PlayerWidget::onOpenLocalMedia()
{
    const QString path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation)
                             .value(0, QDir::homePath());
    const QString fileName
        = QFileDialog::getOpenFileName(this,
                                       tr("Open File"),
                                       path,
                                       tr("Audio Video (*.mp3 *.mp4 *.mkv *.rmvb)"));
    if (fileName.isEmpty())
        return;

    emit openFile(fileName);
}

void PlayerWidget::onOpenWebMedia()
{
    QDialog dialog(this);
    QLineEdit *lineEdit = new QLineEdit(&dialog);
    connect(lineEdit, &QLineEdit::returnPressed, &dialog, &QDialog::accept);
    lineEdit->setPlaceholderText("http://.....");
    QHBoxLayout *layout = new QHBoxLayout(&dialog);
    layout->setContentsMargins(QMargins());
    layout->setSpacing(0);
    layout->addWidget(lineEdit);
    dialog.setMinimumWidth(width() / 2);
    dialog.exec();

    const QString str(lineEdit->text().trimmed());
    if (str.isEmpty()) {
        return;
    }
    QUrl url(str);
    if (!url.isValid()) {
        qWarning("Invalid URL: %s", qUtf8Printable(url.toString()));
        return;
    }

    emit openFile(url.toEncoded());
}

void PlayerWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QWidget::contextMenuEvent(event);
    d_ptr->menu->exec(event->globalPos());
}
