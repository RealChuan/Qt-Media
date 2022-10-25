#ifndef VIDEOOUPUTRENDERWIDGET_HPP
#define VIDEOOUPUTRENDERWIDGET_HPP

#include "videooutputrender.hpp"

#include <QWidget>

namespace Ffmpeg {

class FFMPEG_EXPORT VideoOuputRenderWidget : public VideoOutputRender, public QWidget
{
public:
    explicit VideoOuputRenderWidget(QWidget *parent = nullptr);

    void setDisplayImage(const QImage &image) override;
    void onSubtitleImages(const QVector<Ffmpeg::SubtitleImage> &) override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

} // namespace Ffmpeg

#endif // VIDEOOUPUTRENDERWIDGET_HPP
