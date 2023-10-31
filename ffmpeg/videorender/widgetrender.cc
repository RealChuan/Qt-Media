#include "widgetrender.hpp"

#include <ffmpeg/frame.hpp>
#include <ffmpeg/subtitle.h>
#include <ffmpeg/videoformat.hpp>
#include <ffmpeg/videoframeconverter.hpp>

#include <QPainter>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

class WidgetRender::WidgetRenderPrivate
{
public:
    explicit WidgetRenderPrivate(QWidget *parent)
        : owner(parent)
    {}
    ~WidgetRenderPrivate() = default;

    QWidget *owner;

    QSizeF size;
    QRectF frameRect;
    QSharedPointer<Frame> framePtr;
    // Rendering is best optimized to the Format_RGB32 and Format_ARGB32_Premultiplied formats
    //QList<AVPixelFormat> supportFormats = VideoFormat::qFormatMaps.keys();
    QList<AVPixelFormat> supportFormats = {AV_PIX_FMT_RGB32};
    QScopedPointer<VideoFrameConverter> frameConverterPtr;
    QSharedPointer<Subtitle> subTitleFramePtr;
    QImage videoImage;
    QImage subTitleImage;

    QColor backgroundColor = Qt::black;
};

WidgetRender::WidgetRender(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new WidgetRenderPrivate(this))
{}

WidgetRender::~WidgetRender() = default;

bool WidgetRender::isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt)
{
    return d_ptr->supportFormats.contains(pix_fmt);
}

QSharedPointer<Frame> WidgetRender::convertSupported_pix_fmt(QSharedPointer<Frame> frame)
{
    auto dst_pix_fmt = AV_PIX_FMT_RGB32;
    auto avframe = frame->avFrame();
    auto size = QSize(avframe->width, avframe->height);
    size.scale(this->size() * devicePixelRatio(), Qt::KeepAspectRatio);
    if (d_ptr->frameConverterPtr.isNull()) {
        d_ptr->frameConverterPtr.reset(new VideoFrameConverter(frame.data(), size, dst_pix_fmt));
    } else {
        d_ptr->frameConverterPtr->flush(frame.data(), size, dst_pix_fmt);
    }
    QSharedPointer<Frame> frameRgbPtr(new Frame);
    frameRgbPtr->imageAlloc(size, dst_pix_fmt);
    d_ptr->frameConverterPtr->scale(frame.data(), frameRgbPtr.data());
    //    qDebug() << frameRgbPtr->avFrame()->width << frameRgbPtr->avFrame()->height
    //             << frameRgbPtr->avFrame()->format;
    return frameRgbPtr;
}

QVector<AVPixelFormat> WidgetRender::supportedOutput_pix_fmt()
{
    return d_ptr->supportFormats;
}

void WidgetRender::resetAllFrame()
{
    d_ptr->videoImage = QImage();
    d_ptr->subTitleImage = QImage();
    d_ptr->framePtr.reset();
    d_ptr->subTitleFramePtr.reset();
}

QWidget *WidgetRender::widget()
{
    return this;
}

void WidgetRender::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    // draw BackGround
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    painter.drawRect(rect());

    if (d_ptr->videoImage.isNull()) {
        return;
    }

    auto size = d_ptr->videoImage.size().scaled(this->size(), Qt::KeepAspectRatio);
    auto rect = QRect((width() - size.width()) / 2,
                      (height() - size.height()) / 2,
                      size.width(),
                      size.height());
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    // draw VideoImage
    painter.drawImage(rect, d_ptr->videoImage);
    // draw SubTitlImage
    paintSubTitleFrame(rect, &painter);
}

void WidgetRender::updateFrame(QSharedPointer<Frame> frame)
{
    QMetaObject::invokeMethod(
        this, [=] { displayFrame(frame); }, Qt::QueuedConnection);
}

void WidgetRender::updateSubTitleFrame(QSharedPointer<Subtitle> frame)
{
    // Rendering is best optimized to the Format_RGB32 and Format_ARGB32_Premultiplied formats
    auto img = frame->image().convertedTo(QImage::Format_ARGB32_Premultiplied);
    img = img.scaled(size() * devicePixelRatio(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    img.setDevicePixelRatio(devicePixelRatio());
    QMetaObject::invokeMethod(
        this,
        [=] {
            d_ptr->subTitleFramePtr = frame;
            d_ptr->subTitleImage = img;
            // need update?
            //update();
        },
        Qt::QueuedConnection);
}

void WidgetRender::displayFrame(QSharedPointer<Frame> framePtr)
{
    d_ptr->framePtr = framePtr;
    d_ptr->videoImage = framePtr->toImage();
    d_ptr->videoImage.setDevicePixelRatio(devicePixelRatio());
    update();
}

void WidgetRender::paintSubTitleFrame(const QRect &rect, QPainter *painter)
{
    if (d_ptr->subTitleImage.isNull()) {
        return;
    }
    if (d_ptr->subTitleFramePtr->pts() > d_ptr->framePtr->pts()
        || (d_ptr->subTitleFramePtr->pts() + d_ptr->subTitleFramePtr->duration())
               < d_ptr->framePtr->pts()) {
        return;
    }
    painter->drawImage(rect, d_ptr->subTitleImage);
}

} // namespace Ffmpeg
