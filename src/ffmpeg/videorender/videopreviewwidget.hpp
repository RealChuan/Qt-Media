#pragma once

#include <ffmpeg/frame.hpp>

#include <QWidget>

#include <ffmpeg/ffmepg_global.h>

namespace Ffmpeg {

class FFMPEG_EXPORT VideoPreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoPreviewWidget(QWidget *parent = nullptr);
    ~VideoPreviewWidget() override;

    void startPreview(const QString &filepath, int videoIndex, qint64 timestamp, qint64 duration);
    void clearAllTask();

    void setDisplayImage(const FramePtr &framePtr,
                         const QImage &image,
                         qint64 pts,
                         const QString &chapterText);
    void setDisplayText(const QString &text);

    [[nodiscard]] auto currentTaskId() const -> int;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void paintWaiting(QPainter *painter);
    void paintImage(QPainter *painter);
    void paintText(QPainter *painter);

    class VideoPreviewWidgetPrivate;
    QScopedPointer<VideoPreviewWidgetPrivate> d_ptr;
};

} // namespace Ffmpeg
