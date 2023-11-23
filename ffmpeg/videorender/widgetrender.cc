#include "widgetrender.hpp"

#include <ffmpeg/ffmpegutils.hpp>
#include <ffmpeg/frame.hpp>
#include <ffmpeg/subtitle.h>
#include <ffmpeg/videoformat.hpp>
#include <ffmpeg/videoframeconverter.hpp>
#include <filter/filter.hpp>
#include <filter/filtercontext.hpp>

#include <QPainter>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

namespace Ffmpeg {

struct FrameParam
{
    FrameParam() = default;

    explicit FrameParam(Frame *frame)
    {
        auto *avFrame = frame->avFrame();
        size = QSize(avFrame->width, avFrame->height);
        format = avFrame->format;
        time_base = avFrame->time_base;
        sample_aspect_ratio = avFrame->sample_aspect_ratio;
        sample_rate = avFrame->sample_rate;
        ch_layout = avFrame->ch_layout;
    }

    auto operator==(const FrameParam &other) const -> bool
    {
        return size == other.size && format == other.format
               && compareAVRational(time_base, other.time_base)
               && compareAVRational(sample_aspect_ratio, other.sample_aspect_ratio)
               && sample_rate == other.sample_rate
               && av_channel_layout_compare(&ch_layout, &other.ch_layout) == 0;
    }

    auto operator!=(const FrameParam &other) const -> bool { return !(*this == other); }

    QSize size;
    int format;
    AVRational time_base;
    AVRational sample_aspect_ratio;
    int sample_rate;
    AVChannelLayout ch_layout;
};

class WidgetRender::WidgetRenderPrivate
{
public:
    explicit WidgetRenderPrivate(WidgetRender *q)
        : q_ptr(q)
    {}

    ~WidgetRenderPrivate() = default;

    auto swsScale(const FramePtr &framePtr) -> FramePtr
    {
        auto dst_pix_fmt = AV_PIX_FMT_RGB32;
        auto *avframe = framePtr->avFrame();
        auto size = QSize(avframe->width, avframe->height);
        size.scale(q_ptr->size() * q_ptr->devicePixelRatio(), Qt::KeepAspectRatio);
        if (frameConverterPtr.isNull()) {
            frameConverterPtr.reset(new VideoFrameConverter(framePtr.data(), size, dst_pix_fmt));
        } else {
            frameConverterPtr->flush(framePtr.data(), size, dst_pix_fmt);
        }
        frameConverterPtr->setColorspaceDetails(framePtr.data(),
                                                q_ptr->m_colorSpaceTrc.brightness,
                                                q_ptr->m_colorSpaceTrc.contrast,
                                                q_ptr->m_colorSpaceTrc.saturation);
        QSharedPointer<Frame> frameRgbPtr(new Frame);
        frameRgbPtr->imageAlloc(size, dst_pix_fmt);
        frameConverterPtr->scale(framePtr.data(), frameRgbPtr.data());
        //    qDebug() << frameRgbPtr->avFrame()->width << frameRgbPtr->avFrame()->height
        //             << frameRgbPtr->avFrame()->format;
        return frameRgbPtr;
    }

    WidgetRender *q_ptr;

    QSizeF size;
    QRectF frameRect;
    QSharedPointer<Frame> framePtr;
    // Rendering is best optimized to the Format_RGB32 and Format_ARGB32_Premultiplied formats
    //QList<AVPixelFormat> supportFormats = VideoFormat::qFormatMaps.keys();
    QList<AVPixelFormat> supportFormats = {AV_PIX_FMT_RGB32};
    QScopedPointer<VideoFrameConverter> frameConverterPtr;
    QScopedPointer<Filter> filterPtr;
    QSharedPointer<Subtitle> subTitleFramePtr;
    QImage videoImage;
    QImage subTitleImage;

    QColor backgroundColor = Qt::black;

    Tonemap::Type tonemapType;
};

WidgetRender::WidgetRender(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new WidgetRenderPrivate(this))
{}

WidgetRender::~WidgetRender() = default;

auto WidgetRender::isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) -> bool
{
    return d_ptr->supportFormats.contains(pix_fmt);
}

auto WidgetRender::convertSupported_pix_fmt(QSharedPointer<Frame> framePtr) -> QSharedPointer<Frame>
{
    static FrameParam lastFrameParam;
    FrameParam frameParam(framePtr.data());

    static QSize lastScaleSize;
    auto *avframe = framePtr->avFrame();
    auto size = QSize(avframe->width, avframe->height);
    size.scale(this->size() * devicePixelRatio(), Qt::KeepAspectRatio);

    if (d_ptr->framePtr.isNull() || d_ptr->filterPtr.isNull() || lastFrameParam != frameParam
        || d_ptr->tonemapType != m_tonemapType || lastScaleSize != size) {
        d_ptr->filterPtr.reset(new Filter);
        lastFrameParam = frameParam;
        lastScaleSize = size;
    }
    auto isInitialized = d_ptr->filterPtr->isInitialized();
    if (!isInitialized) {
        d_ptr->filterPtr->init(AVMEDIA_TYPE_VIDEO, framePtr.data());
        d_ptr->tonemapType = m_tonemapType;
        QString tonemap("none");
        // need zimg zscale "tonemap=clip" in filterSpec
        switch (d_ptr->tonemapType) {
        case Tonemap::Type::CLIP: tonemap = "clip"; break;
        case Tonemap::Type::LINEAR: tonemap = "linear"; break;
        case Tonemap::Type::GAMMA: tonemap = "gamma"; break;
        case Tonemap::Type::REINHARD: tonemap = "reinhard"; break;
        case Tonemap::Type::HABLE: tonemap = "hable"; break;
        case Tonemap::Type::MOBIUS: tonemap = "mobius"; break;
        default: break;
        }
        auto pix_fmt = AV_PIX_FMT_RGB32;
        av_opt_set_bin(d_ptr->filterPtr->buffersinkCtx()->avFilterContext(),
                       "pix_fmts",
                       reinterpret_cast<uint8_t *>(&pix_fmt),
                       sizeof(pix_fmt),
                       AV_OPT_SEARCH_CHILDREN);
        auto filterSpec = QString("scale=%1:%2")
                              .arg(QString::number(size.width()), QString::number(size.height()));
        d_ptr->filterPtr->config(filterSpec);
        // d_ptr->filterPtr->config("null");
    }
    auto framePtrs = d_ptr->filterPtr->filterFrame(framePtr.data());
    if (framePtrs.isEmpty()) {
        return {};
    }
    // qDebug() << framePtrs.first()->avFrame()->width << framePtrs.first()->avFrame()->height
    //          << framePtrs.first()->avFrame()->format;
    return framePtrs.first();

    // return d_ptr->swsScale(framePtr);
}

auto WidgetRender::supportedOutput_pix_fmt() -> QVector<AVPixelFormat>
{
    return {};
}

void WidgetRender::resetAllFrame()
{
    d_ptr->videoImage = QImage();
    d_ptr->subTitleImage = QImage();
    d_ptr->framePtr.reset();
    d_ptr->subTitleFramePtr.reset();
}

auto WidgetRender::widget() -> QWidget *
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
