#include "videoframeconverter.hpp"
#include "averrormanager.hpp"
#include "codeccontext.h"
#include "frame.hpp"
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
    VideoFrameConverterPrivate(VideoFrameConverter *q)
        : q_ptr(q)
    {}

    inline void debugMessage()
    {
#ifndef QT_NO_DEBUG
        auto support_in = sws_isSupportedInput(src_pix_fmt);
        auto support_out = sws_isSupportedOutput(dst_pix_fmt);
        if (!support_in) {
            qInfo() << QString("support src_pix_fmt: %1 %2")
                           .arg(QString::number(src_pix_fmt), QString::number(support_in));
        }
        if (!support_out) {
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
    auto ctx = codecCtx->avCodecCtx();
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

VideoFrameConverter::VideoFrameConverter(Frame *frame,
                                         const QSize &size,
                                         AVPixelFormat pix_fmt,
                                         QObject *parent)
    : QObject(parent)
    , d_ptr(new VideoFrameConverterPrivate(this))
{
    flush(frame, size, pix_fmt);
}

VideoFrameConverter::~VideoFrameConverter()
{
    Q_ASSERT(d_ptr->swsContext != nullptr);
    sws_freeContext(d_ptr->swsContext);
}

void VideoFrameConverter::flush(Frame *frame, const QSize &dstSize, AVPixelFormat pix_fmt)
{
    auto avFrame = frame->avFrame();
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

int VideoFrameConverter::scale(Frame *in, Frame *out)
{
    Q_ASSERT(d_ptr->swsContext != nullptr);
    auto inFrame = in->avFrame();
    auto outFrame = out->avFrame();
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
    out->setPts(in->pts());
    out->setDuration(in->duration());
    return ret;
}

bool VideoFrameConverter::isSupportedInput_pix_fmt(AVPixelFormat pix_fmt)
{
    return sws_isSupportedInput(pix_fmt);
}

bool VideoFrameConverter::isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt)
{
    return sws_isSupportedOutput(pix_fmt);
}

} // namespace Ffmpeg
