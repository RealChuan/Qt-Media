#include "transcodercontext.hpp"
#include "audiofifo.hpp"
#include "audioframeconverter.h"
#include "avcontextinfo.h"
#include "codeccontext.h"

#include <filter/filter.hpp>
#include <filter/filtercontext.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

namespace Ffmpeg {

auto TranscoderContext::initFilter(const QString &filter_spec,
                                   const QSharedPointer<Frame> &framePtr) const -> bool
{
    if (filterPtr->isInitialized()) {
        return true;
    }

    auto *enc_ctx = encContextInfoPtr->codecCtx()->avCodecCtx();
    switch (decContextInfoPtr->mediaType()) {
    case AVMEDIA_TYPE_VIDEO: {
        filterPtr->init(AVMEDIA_TYPE_VIDEO, framePtr.data());
        auto pix_fmt = encContextInfoPtr->pixfmt();
        av_opt_set_bin(filterPtr->buffersinkCtx()->avFilterContext(),
                       "pix_fmts",
                       reinterpret_cast<uint8_t *>(&pix_fmt),
                       sizeof(pix_fmt),
                       AV_OPT_SEARCH_CHILDREN);
    } break;
    case AVMEDIA_TYPE_AUDIO: {
        filterPtr->init(AVMEDIA_TYPE_AUDIO, framePtr.data());
        auto *avFilterContext = filterPtr->buffersinkCtx()->avFilterContext();
        av_opt_set_bin(avFilterContext,
                       "sample_rates",
                       reinterpret_cast<uint8_t *>(&enc_ctx->sample_rate),
                       sizeof(enc_ctx->sample_rate),
                       AV_OPT_SEARCH_CHILDREN);
        av_opt_set_bin(avFilterContext,
                       "sample_fmts",
                       reinterpret_cast<uint8_t *>(&enc_ctx->sample_fmt),
                       sizeof(enc_ctx->sample_fmt),
                       AV_OPT_SEARCH_CHILDREN);
        av_opt_set(avFilterContext,
                   "ch_layouts",
                   getAVChannelLayoutDescribe(enc_ctx->ch_layout).toUtf8().data(),
                   AV_OPT_SEARCH_CHILDREN);
    } break;
    default: return false;
    }

    filterPtr->config(filter_spec);
    return true;
}

} // namespace Ffmpeg
