#ifndef WIDGETRENDER_HPP
#define WIDGETRENDER_HPP

#include "videorender.hpp"

#include <QWidget>

namespace Ffmpeg {

class FFMPEG_EXPORT WidgetRender : public VideoRender, public QWidget
{
public:
    explicit WidgetRender(QWidget *parent = nullptr);
    ~WidgetRender() override;

    bool isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) override;
    QSharedPointer<Frame> convertSupported_pix_fmt(QSharedPointer<Frame> frame) override;
    QVector<AVPixelFormat> supportedOutput_pix_fmt() override;

    void resetAllFrame() override;

protected:
    void paintEvent(QPaintEvent *event) override;

    void updateFrame(QSharedPointer<Frame> frame) override;
    void updateSubTitleFrame(QSharedPointer<Subtitle> frame) override;

private:
    void displayFrame(QSharedPointer<Frame> framePtr);
    void paintSubTitleFrame(const QRect &rect, QPainter *painter);

    class WidgetRenderPrivate;
    QScopedPointer<WidgetRenderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // WIDGETRENDER_HPP
