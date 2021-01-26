#include "playerwidget.h"

#include <QtWidgets>

class PlayerWidgetPrivate{
public:
    PlayerWidgetPrivate(QWidget *parent)
        : owner(parent){
        menu = new QMenu(owner);
    }
    QWidget *owner;

    QMenu *menu;
    QPixmap pixmap;
};

PlayerWidget::PlayerWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , d_ptr(new PlayerWidgetPrivate(this))
{
    QPalette p = palette();
    p.setColor(QPalette::Window, QColor(13,14,17));
    setPalette(p);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
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

void PlayerWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    //if(d_ptr->pixmap.width() > width() || d_ptr->pixmap.height() > height()){
    //    d_ptr->pixmap = d_ptr->pixmap.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    //}
    //int x = (width() - d_ptr->pixmap.width()) / 2;
    //int y = (height() - d_ptr->pixmap.height()) / 2;
    //painter.drawPixmap(QRect(x, y, d_ptr->pixmap.width(), d_ptr->pixmap.height()), d_ptr->pixmap);


    if(d_ptr->pixmap.width() > width() || d_ptr->pixmap.height() > height()){
        double wScale = d_ptr->pixmap.width() * 1.0 / width();
        double hScale = d_ptr->pixmap.height() * 1.0 / height();
        double maxScale = qMax(wScale, hScale);

        int w = d_ptr->pixmap.width() / maxScale;
        int h = d_ptr->pixmap.height() / maxScale;
        int x = (width() - w) / 2;
        int y = (height() - h) / 2;
        painter.drawPixmap(QRect(x, y, w, h), d_ptr->pixmap);
    }else{
        int x = (width() - d_ptr->pixmap.width()) / 2;
        int y = (height() - d_ptr->pixmap.height()) / 2;
        painter.drawPixmap(QRect(x, y, d_ptr->pixmap.width(), d_ptr->pixmap.height()), d_ptr->pixmap);
    }
}

void PlayerWidget::setupUI()
{
    d_ptr->menu->addAction(tr("Open Video"), this, &PlayerWidget::onOpenVideo);
}

void PlayerWidget::onReadyRead(const QPixmap &pixmap)
{
    if(pixmap.isNull()){
        qWarning() << "pixmap is NUll";
        return;
    }
    d_ptr->pixmap = pixmap;
    update();
}

void PlayerWidget::onOpenVideo()
{
    QString path = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).value(0, QDir::homePath());
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),path,
                                                    tr("Video (*.mp4 *.mkv *.rmvb)"));
    emit openFile(fileName);
}

void PlayerWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QWidget::contextMenuEvent(event);
    d_ptr->menu->exec(event->globalPos());
}
