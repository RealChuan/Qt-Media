#ifndef VIDEOPREVIEWWIDGET_HPP
#define VIDEOPREVIEWWIDGET_HPP

#include <QtCore/qglobal.h>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QWidget>
#else
#include <QtGui/QWidget>
#endif

#include <ffmpeg/ffmepg_global.h>

namespace Ffmpeg {

class Frame;

class FFMPEG_EXPORT VideoPreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoPreviewWidget(QWidget *parent = nullptr);
    VideoPreviewWidget(const QString &filepath,
                       int videoIndex,
                       qint64 timestamp,
                       qint64 duration,
                       QWidget *parent = nullptr);
    ~VideoPreviewWidget();

    void startPreview(const QString &filepath, int videoIndex, qint64 timestamp, qint64 duration);

    void setDisplayImage(QSharedPointer<Ffmpeg::Frame> frame, const QImage &image, qint64 pts);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct VideoPreviewWidgetPrivate;
    QScopedPointer<VideoPreviewWidgetPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // VIDEOPREVIEWWIDGET_HPP
