#include "videoouputrenderwidget.hpp"

#include <QDebug>
#include <QPainter>

namespace Ffmpeg {

VideoOuputRenderWidget::VideoOuputRenderWidget(QWidget *parent)
    : QWidget(parent)
{
    //QPalette p = palette();
    //p.setColor(QPalette::Window, QColor(13,14,17));
    //setPalette(p);
    //setAttribute(Qt::WA_OpaquePaintEvent);
    //setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
}

void VideoOuputRenderWidget::setDisplayImage(const QImage &image)
{
    if (image.isNull()) {
        qWarning() << "image is null!";
        return;
    }
    m_image = image;
    checkSubtitle();
    QMetaObject::invokeMethod(
        this, [this] { update(); }, Qt::QueuedConnection);
}

void VideoOuputRenderWidget::onSubtitleImages(const QVector<SubtitleImage> &subtitleImages)
{
    m_subtitleImages = subtitleImages;
    checkSubtitle();
    update();
}

void VideoOuputRenderWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    //QElapsedTimer timer;
    //timer.start();

    QPainter painter(this);
    drawBackGround(painter, rect());
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    drawVideoImage(painter, rect());

    //qDebug() << timer.elapsed();
}

} // namespace Ffmpeg
