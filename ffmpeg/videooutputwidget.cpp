#include "decoderaudioframe.h"
#include "videooutputwidget.h"

#include <QtWidgets>

namespace Ffmpeg {

class VideoOutputWidgetPrivate{
public:
    VideoOutputWidgetPrivate(QWidget *parent)
        : owner(parent){ }

    QWidget *owner;
    QImage image;
    QVector<SubtitleImage> subtitleImages;
};

VideoOutputWidget::VideoOutputWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , d_ptr(new VideoOutputWidgetPrivate(this))
{
    //QPalette p = palette();
    //p.setColor(QPalette::Window, QColor(13,14,17));
    //setPalette(p);
    //setAttribute(Qt::WA_OpaquePaintEvent);
    //setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
}

VideoOutputWidget::~VideoOutputWidget()
{

}

void VideoOutputWidget::onReadyRead(const QImage &image)
{
    if(image.isNull()){
        qWarning() << "image is null!";
        return;
    }
    d_ptr->image = image;
    checkSubtitle();
    update();
}

void VideoOutputWidget::onSubtitleImages(const QVector<SubtitleImage> &subtitleImages)
{
    d_ptr->subtitleImages = subtitleImages;
    checkSubtitle();
    update();
}

void VideoOutputWidget::onFinish()
{
    d_ptr->image = QImage();
    update();
}

void VideoOutputWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QElapsedTimer timer;
    timer.start();

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    drawBackGround(painter);
    drawVideoImage(painter);

    //qDebug() << timer.elapsed();
}

void VideoOutputWidget::drawBackGround(QPainter &painter)
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    painter.drawRect(rect());
}

void VideoOutputWidget::drawVideoImage(QPainter &painter)
{
    if(d_ptr->image.isNull())
        return;

    //scaled 太吃性能，不过效果好
    //if(d_ptr->pixmap.width() > width() || d_ptr->pixmap.height() > height()){
    //    d_ptr->pixmap = d_ptr->pixmap.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    //}
    //int x = (width() - d_ptr->pixmap.width()) / 2;
    //int y = (height() - d_ptr->pixmap.height()) / 2;
    //painter.drawPixmap(QRect(x, y, d_ptr->pixmap.width(), d_ptr->pixmap.height()), d_ptr->pixmap);

    if(d_ptr->image.width() > width() || d_ptr->image.height() > height()){
        double wScale = d_ptr->image.width() * 1.0 / width();
        double hScale = d_ptr->image.height() * 1.0 / height();
        double maxScale = qMax(wScale, hScale);

        double w = d_ptr->image.width() / maxScale;
        double h = d_ptr->image.height() / maxScale;
        double x = (width() - w) / 2;
        double y = (height() - h) / 2;
        painter.drawImage(QRect(x, y, w, h), d_ptr->image);
    }else{
        double x = (width() - d_ptr->image.width()) / 2;
        double y = (height() - d_ptr->image.height()) / 2;
        painter.drawImage(QRect(x, y, d_ptr->image.width(), d_ptr->image.height()), d_ptr->image);
    }
}

void VideoOutputWidget::checkSubtitle()
{
    if(d_ptr->subtitleImages.isEmpty())
        return;
    if(d_ptr->image.isNull())
        return;

    double pts = DecoderAudioFrame::audioClock() * 1000;
    if(pts < d_ptr->subtitleImages.at(0).startDisplayTime
        || pts > d_ptr->subtitleImages.at(0).endDisplayTime){
        return;
    }

    QPainter painter(&d_ptr->image);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    for(const SubtitleImage& subtitleimage: d_ptr->subtitleImages){
        QRectF rectF = subtitleimage.rectF;
        if(!rectF.isValid()){
            rectF = QRectF(0, d_ptr->image.height() / 12.0 * 11, d_ptr->image.width(), d_ptr->image.height() / 12.0);
        }
        if(subtitleimage.image.isNull()){
            QFont font = painter.font();
            font.setPixelSize(d_ptr->image.height() / 13.0);
            painter.setFont(font);
            painter.setPen(Qt::white);
            painter.drawText(rectF, Qt::AlignCenter, subtitleimage.text);
        }else{
            painter.drawImage(rectF, subtitleimage.image);
        }
    }

}

}
