#include "videooutputrender.hpp"
#include "decoderaudioframe.h"

#include <QPainter>

namespace Ffmpeg {

void VideoOutputRender::onFinish()
{
    m_image = QImage();
    m_subtitleImages.clear();
}

void VideoOutputRender::drawBackGround(QPainter &painter, const QRect &rect)
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    painter.drawRect(rect);
}

void VideoOutputRender::drawVideoImage(QPainter &painter, const QRect &rect)
{
    if (m_image.isNull()) {
        return;
    }

    //scaled 太吃性能，不过效果好
    //if(d_ptr->pixmap.width() > width() || d_ptr->pixmap.height() > height()){
    //    d_ptr->pixmap = d_ptr->pixmap.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    //}
    //int x = (width() - d_ptr->pixmap.width()) / 2;
    //int y = (height() - d_ptr->pixmap.height()) / 2;
    //painter.drawPixmap(QRect(x, y, d_ptr->pixmap.width(), d_ptr->pixmap.height()), d_ptr->pixmap);

    if (m_image.width() > rect.width() || m_image.height() > rect.height()) {
        double wScale = m_image.width() * 1.0 / rect.width();
        double hScale = m_image.height() * 1.0 / rect.height();
        double maxScale = qMax(wScale, hScale);

        double w = m_image.width() / maxScale;
        double h = m_image.height() / maxScale;
        double x = (rect.width() - w) / 2;
        double y = (rect.height() - h) / 2;
        painter.drawImage(QRect(x, y, w, h), m_image);
    } else {
        double x = (rect.width() - m_image.width()) / 2;
        double y = (rect.height() - m_image.height()) / 2;
        painter.drawImage(QRect(x, y, m_image.width(), m_image.height()), m_image);
    }
}

void VideoOutputRender::checkSubtitle()
{
    if (m_subtitleImages.isEmpty() || m_image.isNull()) {
        return;
    }

    double pts = DecoderAudioFrame::audioClock() * 1000;
    if (pts < m_subtitleImages.at(0).startDisplayTime
        || pts > m_subtitleImages.at(0).endDisplayTime) {
        return;
    }

    QPainter painter(&m_image);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    for (const SubtitleImage &subtitleimage : qAsConst(m_subtitleImages)) {
        QRectF rectF = subtitleimage.rectF;
        if (!rectF.isValid()) {
            double h = m_image.rect().height() / 20.0;
            rectF = m_image.rect().adjusted(0, h, 0, -h);
        }
        if (subtitleimage.image.isNull()) {
            QFont font = painter.font();
            font.setPixelSize(m_image.height() / 13.0);
            painter.setFont(font);
            painter.setPen(Qt::white);
            painter.drawText(rectF, Qt::AlignHCenter | Qt::AlignBottom, subtitleimage.text);
        } else {
            painter.drawImage(rectF, subtitleimage.image);
        }
    }
}

} // namespace Ffmpeg
