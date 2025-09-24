#include "videoframeconverter.hpp"
#include "averrormanager.hpp"
#include "codeccontext.h"
#include "videoformat.hpp"

#include <QDebug>
#include <QImage>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace Ffmpeg {

class VideoFrameConverter::VideoFrameConverterPrivate
{
public:
    explicit VideoFrameConverterPrivate(VideoFrameConverter *q)
        : q_ptr(q)
    {}

    inline void debugMessage() const
    {
#ifndef QT_NO_DEBUG
        auto support_in = sws_isSupportedInput(src_pix_fmt);
        auto support_out = sws_isSupportedOutput(dst_pix_fmt);
        if (support_in == 0) {
            qInfo() << QString("support src_pix_fmt: %1 %2")
                           .arg(QString::number(src_pix_fmt), QString::number(support_in));
        }
        if (support_out == 0) {
            qInfo() << QString("support dst_pix_fmt: %1 %2")
                           .arg(QString::number(dst_pix_fmt), QString::number(support_out));
        }
#endif
    }

    VideoFrameConverter *q_ptr;

    struct SwsContext *swsContext = nullptr;
    AVPixelFormat src_pix_fmt = AVPixelFormat::AV_PIX_FMT_NONE;
    AVPixelFormat dst_pix_fmt = AVPixelFormat::AV_PIX_FMT_NONE;
    QSize dstSize = {-1, -1};
};

VideoFrameConverter::VideoFrameConverter(CodecContext *codecCtx,
                                         const QSize &size,
                                         AVPixelFormat pix_fmt,
                                         QObject *parent)
    : QObject(parent)
    , d_ptr(new VideoFrameConverterPrivate(this))
{
    auto *ctx = codecCtx->avCodecCtx();
    d_ptr->src_pix_fmt = ctx->pix_fmt;
    d_ptr->dst_pix_fmt = pix_fmt;
    d_ptr->debugMessage();
    size.isValid() ? d_ptr->dstSize = size : d_ptr->dstSize = QSize(ctx->width, ctx->height);
    d_ptr->swsContext = sws_getCachedContext(d_ptr->swsContext,
                                             ctx->width,
                                             ctx->height,
                                             d_ptr->src_pix_fmt,
                                             d_ptr->dstSize.width(),
                                             d_ptr->dstSize.height(),
                                             d_ptr->dst_pix_fmt,
                                             d_ptr->dstSize.width() > ctx->width ? SWS_BICUBIC
                                                                                 : SWS_BILINEAR,
                                             nullptr,
                                             nullptr,
                                             nullptr);
    Q_ASSERT(d_ptr->swsContext != nullptr);
}

VideoFrameConverter::VideoFrameConverter(const FramePtr &framePtr,
                                         const QSize &size,
                                         AVPixelFormat pix_fmt,
                                         QObject *parent)
    : QObject(parent)
    , d_ptr(new VideoFrameConverterPrivate(this))
{
    flush(framePtr, size, pix_fmt);
}

VideoFrameConverter::~VideoFrameConverter()
{
    Q_ASSERT(d_ptr->swsContext != nullptr);
    sws_freeContext(d_ptr->swsContext);
}

void VideoFrameConverter::setColorspaceDetails(const FramePtr &framePtr,
                                               float brightness,
                                               float contrast,
                                               float saturation)
{
    auto *avFrame = framePtr->avFrame();
    int srcRange = 0;
    switch (avFrame->color_range) {
    case AVCOL_RANGE_MPEG: srcRange = 0; break;
    default: srcRange = 1; break;
    }
    sws_setColorspaceDetails(d_ptr->swsContext,
                             sws_getCoefficients(avFrame->colorspace),
                             srcRange,
                             sws_getCoefficients(SWS_CS_DEFAULT),
                             1,
                             static_cast<int>(brightness * (1 << 16)),
                             static_cast<int>(contrast * (1 << 16)),
                             static_cast<int>(saturation * (1 << 16)));
}

void VideoFrameConverter::flush(const FramePtr &framePtr,
                                const QSize &dstSize,
                                AVPixelFormat pix_fmt)
{
    auto *avFrame = framePtr->avFrame();
    d_ptr->src_pix_fmt = static_cast<AVPixelFormat>(avFrame->format);
    d_ptr->dst_pix_fmt = pix_fmt;
    d_ptr->debugMessage();
    dstSize.isValid() ? d_ptr->dstSize = dstSize
                      : d_ptr->dstSize = QSize(avFrame->width, avFrame->height);
    d_ptr->swsContext = sws_getCachedContext(d_ptr->swsContext,
                                             avFrame->width,
                                             avFrame->height,
                                             d_ptr->src_pix_fmt,
                                             d_ptr->dstSize.width(),
                                             d_ptr->dstSize.height(),
                                             d_ptr->dst_pix_fmt,
                                             d_ptr->dstSize.width() > avFrame->width ? SWS_BICUBIC
                                                                                     : SWS_BILINEAR,
                                             nullptr,
                                             nullptr,
                                             nullptr);
    Q_ASSERT(d_ptr->swsContext != nullptr);
}

auto VideoFrameConverter::scale(const FramePtr &inPtr, const FramePtr &outPtr) -> int
{
    Q_ASSERT(d_ptr->swsContext != nullptr);
    auto *inFrame = inPtr->avFrame();
    auto *outFrame = outPtr->avFrame();
    auto ret = sws_scale(d_ptr->swsContext,
                         static_cast<const unsigned char *const *>(inFrame->data),
                         inFrame->linesize,
                         0,
                         inFrame->height,
                         outFrame->data,
                         outFrame->linesize);
    if (ret < 0) {
        SET_ERROR_CODE(ret);
    }
    outFrame->width = d_ptr->dstSize.width();
    outFrame->height = d_ptr->dstSize.height();
    outFrame->format = d_ptr->dst_pix_fmt;
    outFrame->pts = inFrame->pts;
    outPtr->setPts(inPtr->pts());
    outPtr->setDuration(inPtr->duration());
    return ret;
}

auto VideoFrameConverter::isSupportedInput_pix_fmt(AVPixelFormat pix_fmt) -> bool
{
    return sws_isSupportedInput(pix_fmt) != 0;
}

auto VideoFrameConverter::isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) -> bool
{
    return sws_isSupportedOutput(pix_fmt) != 0;
}

} // namespace Ffmpeg
