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

    auto swsScaleFrame(const FramePtr &framePtr) -> FramePtr
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
                                                q_ptr->m_equalizer.ffBrightness(),
                                                q_ptr->m_equalizer.ffContrast(),
                                                q_ptr->m_equalizer.ffSaturation());
        QSharedPointer<Frame> frameRgbPtr(new Frame);
        frameRgbPtr->imageAlloc(size, dst_pix_fmt);
        frameConverterPtr->scale(framePtr.data(), frameRgbPtr.data());
        //    qDebug() << frameRgbPtr->avFrame()->width << frameRgbPtr->avFrame()->height
        //             << frameRgbPtr->avFrame()->format;
        return frameRgbPtr;
    }

    auto fliterFrame(const FramePtr &framePtr) -> FramePtr
    {
        static FrameParam lastFrameParam;
        FrameParam frameParam(framePtr.data());

        auto *avframe = framePtr->avFrame();
        auto size = QSize(avframe->width, avframe->height);
        size.scale(q_ptr->size() * q_ptr->devicePixelRatio(), Qt::KeepAspectRatio);

        if (framePtr.isNull() || filterPtr.isNull() || lastFrameParam != frameParam
            || lastScaleSize != size || equalizer != q_ptr->m_equalizer
            /*|| tonemapType != q_ptr->m_tonemapType || destPrimaries != q_ptr->m_destPrimaries*/) {
            filterPtr.reset(new Filter);
            lastFrameParam = frameParam;
            lastScaleSize = size;
            equalizer = q_ptr->m_equalizer;
            destPrimaries = q_ptr->m_destPrimaries;
        }
        if (!filterPtr->isInitialized()) {
            filterPtr->init(AVMEDIA_TYPE_VIDEO, framePtr.data());
            tonemapType = q_ptr->m_tonemapType;
            auto pix_fmt = AV_PIX_FMT_RGB32;
            av_opt_set_bin(filterPtr->buffersinkCtx()->avFilterContext(),
                           "pix_fmts",
                           reinterpret_cast<uint8_t *>(&pix_fmt),
                           sizeof(pix_fmt),
                           AV_OPT_SEARCH_CHILDREN);
            auto filterSpec = QString("%1,%2").arg(Filter::scale(size), Filter::ep(equalizer));
            filterPtr->config(filterSpec);
        }
        auto framePtrs = filterPtr->filterFrame(framePtr.data());
        if (framePtrs.isEmpty()) {
            return {};
        }
        // qDebug() << framePtrs.first()->avFrame()->width << framePtrs.first()->avFrame()->height
        //          << framePtrs.first()->avFrame()->format;
        return framePtrs.first();
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

    QSize lastScaleSize;
    MediaConfig::Equalizer equalizer;
    Tonemap::Type tonemapType;
    ColorUtils::Primaries::Type destPrimaries;
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

auto WidgetRender::convertSupported_pix_fmt(const QSharedPointer<Frame> &framePtr)
    -> QSharedPointer<Frame>
{
    return d_ptr->fliterFrame(framePtr);

    // return d_ptr->swsScaleFrame(framePtr);
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
    painter.setBrush(m_backgroundColor);
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

void WidgetRender::updateFrame(const QSharedPointer<Frame> &framePtr)
{
    QMetaObject::invokeMethod(
        this, [=] { displayFrame(framePtr); }, Qt::QueuedConnection);
}

void WidgetRender::updateSubTitleFrame(const QSharedPointer<Subtitle> &framePtr)
{
    // Rendering is best optimized to the Format_RGB32 and Format_ARGB32_Premultiplied formats
    auto img = framePtr->image().convertedTo(QImage::Format_ARGB32_Premultiplied);
    img = img.scaled(size() * devicePixelRatio(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    img.setDevicePixelRatio(devicePixelRatio());
    QMetaObject::invokeMethod(
        this,
        [=] {
            d_ptr->subTitleFramePtr = framePtr;
            d_ptr->subTitleImage = img;
            // need update?
            //update();
        },
        Qt::QueuedConnection);
}

void WidgetRender::displayFrame(const QSharedPointer<Frame> &framePtr)
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
