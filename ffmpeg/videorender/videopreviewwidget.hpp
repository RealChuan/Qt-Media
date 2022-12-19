#ifndef VIDEOPREVIEWWIDGET_HPP
#define VIDEOPREVIEWWIDGET_HPP

#include <QWidget>

#include <ffmpeg/ffmepg_global.h>

namespace Ffmpeg {

class Frame;

class FFMPEG_EXPORT VideoPreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoPreviewWidget(QWidget *parent = nullptr);
    ~VideoPreviewWidget();

    void startPreview(const QString &filepath, int videoIndex, qint64 timestamp, qint64 duration);

    void setDisplayImage(QSharedPointer<Ffmpeg::Frame> frame, const QImage &image, qint64 pts);

    int currentTaskId() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void paintWaiting(QPainter *painter);
    void paintImage(QPainter *painter);
    void paintTime(QPainter *painter);

    class VideoPreviewWidgetPrivate;
    QScopedPointer<VideoPreviewWidgetPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // VIDEOPREVIEWWIDGET_HPP
