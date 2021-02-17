#ifndef VIDEOOUTPUTWIDGET_H
#define VIDEOOUTPUTWIDGET_H

#include <QWidget>

#include "ffmepg_global.h"
#include "subtitle.h"

namespace Ffmpeg {

class VideoOutputWidgetPrivate;
class FFMPEG_EXPORT VideoOutputWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoOutputWidget(QWidget *parent = nullptr);
    ~VideoOutputWidget();

public slots:
    void onReadyRead(const QImage &image);
    void onSubtitleImages(const QVector<SubtitleImage>&);
    void onFinish();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawBackGround(QPainter &painter);
    void drawVideoImage(QPainter &painter);
    void checkSubtitle();

    QScopedPointer<VideoOutputWidgetPrivate> d_ptr;
};

}

#endif // VIDEOOUTPUTWIDGET_H
