#include "playerwidget.h"

#include <QtWidgets>

PlayerWidget::PlayerWidget(QWidget *parent)
    : QWidget(parent)
{
    QPalette p = palette();
    p.setColor(QPalette::Window, QColor(13,14,17));
    setPalette(p);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    setAcceptDrops(true);
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

    if(m_pixmap.width() > width() || m_pixmap.height() > height())
        m_pixmap = m_pixmap.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    int x = (width() - m_pixmap.width()) / 2;
    int y = (height() - m_pixmap.height()) / 2;
    painter.drawPixmap(QRect(x, y, m_pixmap.width(), m_pixmap.height()), m_pixmap);
}

void PlayerWidget::onReadyRead(const QPixmap &pixmap)
{
    if(pixmap.isNull()){
        qWarning() << "pixmap is NUll";
        return;
    }
    m_pixmap = pixmap;
    update();
}
