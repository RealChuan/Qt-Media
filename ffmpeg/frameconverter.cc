#include "frameconverter.hpp"
#include "averror.h"
#include "codeccontext.h"
#include "playframe.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace Ffmpeg {

struct FrameConverter::FrameConverterPrivate
{
    struct SwsContext *swsContext = nullptr;
};

FrameConverter::FrameConverter(CodecContext *codecCtx, const QSize &size, QObject *parent)
    : QObject(parent)
    , d_ptr(new FrameConverterPrivate)
{
    auto ctx = codecCtx->avCodecCtx();
    qInfo() << ctx->pix_fmt;
    auto pix_fmt = ctx->pix_fmt;
    if (sws_isSupportedInput(pix_fmt) <= 0) {
        pix_fmt = AV_PIX_FMT_NV12;
    }
    int width = ctx->width;
    int height = ctx->height;
    if (size.isValid()) {
        width = size.width();
        height = size.height();
    }
    d_ptr->swsContext = sws_getContext(ctx->width,
                                       ctx->height,
                                       pix_fmt,
                                       width,
                                       height,
                                       AV_PIX_FMT_RGB32,
                                       size.width() > ctx->width ? SWS_BICUBIC : SWS_BILINEAR,
                                       NULL,
                                       NULL,
                                       NULL);
    Q_ASSERT(d_ptr->swsContext != nullptr);
}

FrameConverter::~FrameConverter() {}

void FrameConverter::flush(PlayFrame *frame, const QSize &dstSize)
{
    auto avFrame = frame->avFrame();
    int width = avFrame->width;
    int height = avFrame->height;
    if (dstSize.isValid()) {
        width = dstSize.width();
        height = dstSize.height();
    }
    sws_getCachedContext(d_ptr->swsContext,
                         avFrame->width,
                         avFrame->height,
                         static_cast<AVPixelFormat>(avFrame->format),
                         width,
                         height,
                         AV_PIX_FMT_RGB32,
                         dstSize.width() > avFrame->width ? SWS_BICUBIC : SWS_BILINEAR,
                         NULL,
                         NULL,
                         NULL);
    Q_ASSERT(d_ptr->swsContext != nullptr);
}

int FrameConverter::scale(PlayFrame *in, PlayFrame *out, int height)
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
    return ret;
}

QImage FrameConverter::scaleToImageRgb32(PlayFrame *in,
                                         PlayFrame *out,
                                         CodecContext *codecCtx,
                                         const QSize &dstSize)
{
    Q_ASSERT(d_ptr->swsContext != nullptr);
    scale(in, out, codecCtx->height());

    int width = codecCtx->width();
    int height = codecCtx->height();
    if (dstSize.isValid()) {
        width = dstSize.width();
        height = dstSize.height();
    }
    return QImage((uchar *) out->avFrame()->data[0], width, height, QImage::Format_RGB32);
}

} // namespace Ffmpeg
