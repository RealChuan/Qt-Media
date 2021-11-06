#include "avimage.h"
#include "averror.h"
#include "codeccontext.h"
#include "playframe.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace Ffmpeg {

AVImage::AVImage(CodecContext *codecCtx)
{
    AVCodecContext *ctx = codecCtx->avCodecCtx();
    qInfo() << ctx->pix_fmt;
    AVPixelFormat pix_fmt = ctx->pix_fmt;
    if (sws_isSupportedInput(pix_fmt) <= 0) {
        pix_fmt = AV_PIX_FMT_NV12;
    }
    m_swsContext = sws_getContext(ctx->width,
                                  ctx->height,
                                  pix_fmt,
                                  ctx->width,
                                  ctx->height,
                                  AV_PIX_FMT_RGB32,
                                  SWS_BICUBIC,
                                  NULL,
                                  NULL,
                                  NULL);
    Q_ASSERT(m_swsContext != nullptr);
}

AVImage::~AVImage()
{
    Q_ASSERT(m_swsContext != nullptr);
    sws_freeContext(m_swsContext);
}

void AVImage::flush(PlayFrame *frame)
{
    AVFrame *avFrame = frame->avFrame();
    sws_getCachedContext(m_swsContext,
                         avFrame->width,
                         avFrame->height,
                         static_cast<AVPixelFormat>(avFrame->format),
                         avFrame->width,
                         avFrame->height,
                         AV_PIX_FMT_RGB32,
                         SWS_BICUBIC,
                         NULL,
                         NULL,
                         NULL);
    Q_ASSERT(m_swsContext != nullptr);
}

void AVImage::scale(PlayFrame *in, PlayFrame *out, int height)
{
    flush(in);
    AVFrame *inFrame = in->avFrame();
    AVFrame *outFrame = out->avFrame();
    int ret = sws_scale(m_swsContext,
                        (const unsigned char *const *) inFrame->data,
                        inFrame->linesize,
                        0,
                        height,
                        outFrame->data,
                        outFrame->linesize);
    if (ret < 0) {
        qWarning() << AVError::avErrorString(ret);
    }
}

} // namespace Ffmpeg
