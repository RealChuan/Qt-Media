#include "frameconverter.hpp"
#include "averror.h"
#include "codeccontext.h"
#include "frame.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <QImage>

namespace Ffmpeg {

struct FrameConverter::FrameConverterPrivate
{
    struct SwsContext *swsContext = nullptr;
    AVPixelFormat src_pix_fmt = AVPixelFormat::AV_PIX_FMT_NONE;
    AVPixelFormat dst_pix_fmt = AVPixelFormat::AV_PIX_FMT_NONE;
};

FrameConverter::FrameConverter(CodecContext *codecCtx,
                               const QSize &size,
                               AVPixelFormat pix_fmt,
                               QObject *parent)
    : QObject(parent)
    , d_ptr(new FrameConverterPrivate)
{
    auto ctx = codecCtx->avCodecCtx();
    d_ptr->src_pix_fmt = ctx->pix_fmt;
    d_ptr->dst_pix_fmt = pix_fmt;
    debugMessage();
    int width = ctx->width;
    int height = ctx->height;
    if (size.isValid()) {
        width = size.width();
        height = size.height();
    }
    d_ptr->swsContext = sws_getContext(ctx->width,
                                       ctx->height,
                                       d_ptr->src_pix_fmt,
                                       width,
                                       height,
                                       d_ptr->dst_pix_fmt,
                                       size.width() > ctx->width ? SWS_BICUBIC : SWS_BILINEAR,
                                       NULL,
                                       NULL,
                                       NULL);
    Q_ASSERT(d_ptr->swsContext != nullptr);
}

FrameConverter::FrameConverter(Frame *frame,
                               const QSize &size,
                               AVPixelFormat pix_fmt,
                               QObject *parent)
    : QObject(parent)
    , d_ptr(new FrameConverterPrivate)
{
    auto avFrame = frame->avFrame();
    d_ptr->src_pix_fmt = AVPixelFormat(avFrame->format);
    d_ptr->dst_pix_fmt = pix_fmt;
    debugMessage();
    if (sws_isSupportedInput(d_ptr->src_pix_fmt) <= 0) {
        d_ptr->src_pix_fmt = AV_PIX_FMT_NV12;
    }
    int width = avFrame->width;
    int height = avFrame->height;
    if (size.isValid()) {
        width = size.width();
        height = size.height();
    }
    d_ptr->swsContext = sws_getContext(avFrame->width,
                                       avFrame->height,
                                       d_ptr->src_pix_fmt,
                                       width,
                                       height,
                                       d_ptr->dst_pix_fmt,
                                       size.width() > avFrame->width ? SWS_BICUBIC : SWS_BILINEAR,
                                       NULL,
                                       NULL,
                                       NULL);
    Q_ASSERT(d_ptr->swsContext != nullptr);
}

FrameConverter::~FrameConverter()
{
    Q_ASSERT(d_ptr->swsContext != nullptr);
    sws_freeContext(d_ptr->swsContext);
}

void FrameConverter::flush(Frame *frame, const QSize &dstSize, AVPixelFormat pix_fmt)
{
    auto avFrame = frame->avFrame();
    d_ptr->src_pix_fmt = static_cast<AVPixelFormat>(avFrame->format);
    d_ptr->dst_pix_fmt = pix_fmt;
    debugMessage();
    int width = avFrame->width;
    int height = avFrame->height;
    if (dstSize.isValid()) {
        width = dstSize.width();
        height = dstSize.height();
    }
    sws_getCachedContext(d_ptr->swsContext,
                         avFrame->width,
                         avFrame->height,
                         d_ptr->src_pix_fmt,
                         width,
                         height,
                         d_ptr->dst_pix_fmt,
                         dstSize.width() > avFrame->width ? SWS_BICUBIC : SWS_BILINEAR,
                         NULL,
                         NULL,
                         NULL);
    Q_ASSERT(d_ptr->swsContext != nullptr);
}

int FrameConverter::scale(Frame *in, Frame *out, int height)
{
    Q_ASSERT(d_ptr->swsContext != nullptr);
    auto inFrame = in->avFrame();
    auto outFrame = out->avFrame();
    int ret = sws_scale(d_ptr->swsContext,
                        static_cast<const unsigned char *const *>(inFrame->data),
                        inFrame->linesize,
                        0,
                        height,
                        outFrame->data,
                        outFrame->linesize);
    if (ret < 0) {
        qWarning() << AVError::avErrorString(ret);
    }
    outFrame->width = inFrame->width;
    outFrame->height = inFrame->height;
    outFrame->format = d_ptr->dst_pix_fmt;
    return ret;
}

QImage FrameConverter::scaleToQImage(Frame *in,
                                     Frame *out,
                                     const QSize &dstSize,
                                     QImage::Format format)
{
    Q_ASSERT(d_ptr->swsContext != nullptr);
    auto inFrame = in->avFrame();
    scale(in, out, inFrame->height);

    int width = inFrame->width;
    int height = inFrame->height;
    if (dstSize.isValid()) {
        width = dstSize.width();
        height = dstSize.height();
    }
    return QImage((uchar *) out->avFrame()->data[0], width, height, format);
}

bool FrameConverter::isSupportedInput_pix_fmt(AVPixelFormat pix_fmt)
{
    return sws_isSupportedInput(pix_fmt);
}

bool FrameConverter::isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt)
{
    return sws_isSupportedOutput(pix_fmt);
}

void FrameConverter::debugMessage()
{
#ifndef QT_NO_DEBUG
    auto support_in = sws_isSupportedInput(d_ptr->src_pix_fmt);
    auto support_out = sws_isSupportedOutput(d_ptr->dst_pix_fmt);
    qInfo() << QString("support src_pix_fmt: %1 %2")
                   .arg(QString::number(d_ptr->src_pix_fmt), QString::number(support_in));
    qInfo() << QString("support dst_pix_fmt: %1 %2")
                   .arg(QString::number(d_ptr->dst_pix_fmt), QString::number(support_out));
#endif
}

} // namespace Ffmpeg
