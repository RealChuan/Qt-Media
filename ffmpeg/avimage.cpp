#include "avimage.h"
#include "codeccontext.h"
#include "playframe.h"

extern "C"{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace Ffmpeg {

AVImage::AVImage(CodecContext *codecCtx)
{
    AVCodecContext * ctx = codecCtx->avCodecCtx();
    m_swsContext = sws_getContext(ctx->width, ctx->height, ctx->pix_fmt, ctx->width, ctx->height,
                                  AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
    Q_ASSERT(m_swsContext != nullptr);
}

AVImage::~AVImage()
{
    Q_ASSERT(m_swsContext != nullptr);
    sws_freeContext(m_swsContext);
}

SwsContext *AVImage::swsContext()
{
    Q_ASSERT(m_swsContext != nullptr);
    return m_swsContext;
}

void AVImage::scale(PlayFrame *in, PlayFrame *out, int height)
{
    AVFrame *inFrame = in->avFrame();
    AVFrame *outFrame = out->avFrame();
    sws_scale(m_swsContext,
              (const unsigned char* const*)inFrame->data, inFrame->linesize,
              0, height, outFrame->data, outFrame->linesize);
}

}
