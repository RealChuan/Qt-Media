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

    auto isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) -> bool override;
    auto convertSupported_pix_fmt(const QSharedPointer<Frame> &framePtr)
        -> QSharedPointer<Frame> override;
    auto supportedOutput_pix_fmt() -> QList<AVPixelFormat> override;

    void resetAllFrame() override;

    auto widget() -> QWidget * override;

protected:
    void paintEvent(QPaintEvent *event) override;

    void updateFrame(const QSharedPointer<Frame> &framePtr) override;
    void updateSubTitleFrame(const QSharedPointer<Subtitle> &framePtr) override;

private:
    void displayFrame(const QSharedPointer<Frame> &framePtr);
    void paintSubTitleFrame(const QRect &rect, QPainter *painter);

    class WidgetRenderPrivate;
    QScopedPointer<WidgetRenderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // WIDGETRENDER_HPP
