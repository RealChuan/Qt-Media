#include "transcode.hpp"
#include "avcontextinfo.h"
#include "averror.h"
#include "codeccontext.h"
#include "formatcontext.h"
#include "frame.hpp"
#include "packet.h"

#include <filter/filtercontext.hpp>
#include <filter/filtergraph.hpp>
#include <filter/filterinout.hpp>

#include <QSharedPointer>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
}

namespace Ffmpeg {

struct FilteringContext
{
    QSharedPointer<FilterContext> buffersinkCtx;
    QSharedPointer<FilterContext> buffersrcCtx;
    QSharedPointer<FilterGraph> filterGraph;

    QSharedPointer<Packet> encPacket;
    QSharedPointer<Frame> filteredFrame;
};

struct TranscodeContext
{
    QSharedPointer<AVContextInfo> decContextInfo;
    QSharedPointer<AVContextInfo> encContextInfo;

    QSharedPointer<Frame> decFrame;
};

bool init_filter(FilteringContext *fctx,
                 AVContextInfo *decContextInfo,
                 AVContextInfo *encContextInfo,
                 const char *filter_spec)
{
    QSharedPointer<FilterContext> buffersrc_ctx;
    QSharedPointer<FilterContext> buffersink_ctx;
    QSharedPointer<FilterGraph> filter_graph(new FilterGraph);
    auto dec_ctx = decContextInfo->codecCtx()->avCodecCtx();
    auto enc_ctx = encContextInfo->codecCtx()->avCodecCtx();
    switch (decContextInfo->mediaType()) {
    case AVMEDIA_TYPE_VIDEO: {
        buffersrc_ctx.reset(new FilterContext("buffer"));
        buffersink_ctx.reset(new FilterContext("buffersink"));
        auto args
            = QString::asprintf("video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                                dec_ctx->width,
                                dec_ctx->height,
                                dec_ctx->pix_fmt,
                                dec_ctx->time_base.num,
                                dec_ctx->time_base.den,
                                dec_ctx->sample_aspect_ratio.num,
                                dec_ctx->sample_aspect_ratio.den);
        buffersrc_ctx->create("in", args, filter_graph.data());
        buffersink_ctx->create("out", "", filter_graph.data());
        av_opt_set_bin(buffersink_ctx->avFilterContext(),
                       "pix_fmts",
                       (uint8_t *) &enc_ctx->pix_fmt,
                       sizeof(enc_ctx->pix_fmt),
                       AV_OPT_SEARCH_CHILDREN);
    } break;
    case AVMEDIA_TYPE_AUDIO: {
        buffersrc_ctx.reset(new FilterContext("abuffer"));
        buffersink_ctx.reset(new FilterContext("abuffersink"));
        if (!dec_ctx->channel_layout) {
            dec_ctx->channel_layout = av_get_default_channel_layout(dec_ctx->channels);
        }
        auto args = QString::asprintf(
            "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
            dec_ctx->time_base.num,
            dec_ctx->time_base.den,
            dec_ctx->sample_rate,
            av_get_sample_fmt_name(dec_ctx->sample_fmt),
            dec_ctx->channel_layout);
        buffersrc_ctx->create("in", args, filter_graph.data());
        buffersink_ctx->create("out", "", filter_graph.data());
        av_opt_set_bin(buffersink_ctx->avFilterContext(),
                       "channel_layouts",
                       (uint8_t *) &enc_ctx->channel_layout,
                       sizeof(enc_ctx->channel_layout),
                       AV_OPT_SEARCH_CHILDREN);
        av_opt_set_bin(buffersink_ctx->avFilterContext(),
                       "sample_rates",
                       (uint8_t *) &enc_ctx->sample_rate,
                       sizeof(enc_ctx->sample_rate),
                       AV_OPT_SEARCH_CHILDREN);
    } break;
    default: return false;
    }

    QScopedPointer<FilterInOut> fliterOut(new FilterInOut);
    QScopedPointer<FilterInOut> fliterIn(new FilterInOut);
    auto outputs = fliterOut->avFilterInOut();
    auto inputs = fliterIn->avFilterInOut();
    /* Endpoints for the filter graph. */
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx->avFilterContext();
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx->avFilterContext();
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    filter_graph->parse(filter_spec, fliterIn.data(), fliterOut.data());
    filter_graph->config();

    fctx->buffersrcCtx = buffersrc_ctx;
    fctx->buffersrcCtx = buffersink_ctx;
    fctx->filterGraph = filter_graph;

    return true;
}

class Transcode::TranscodePrivate
{
public:
    TranscodePrivate(QObject *parent)
        : owner(parent)
        , inFormatContext(new FormatContext(owner))
        , outFormatContext(new FormatContext(owner))
    {}

    ~TranscodePrivate() { reset(); }

    bool setInMediaIndex(AVContextInfo *contextInfo, int index)
    {
        contextInfo->setIndex(index);
        contextInfo->setStream(inFormatContext->stream(index));
        return contextInfo->initDecoder(inFormatContext->guessFrameRate(index));
    }

    void reset()
    {
        if (!transcodeContexts.isEmpty()) {
            qDeleteAll(transcodeContexts);
            transcodeContexts.clear();
        }
        if (!filteringContexts.isEmpty()) {
            qDeleteAll(filteringContexts);
            filteringContexts.clear();
        }
    }

    QObject *owner;

    FormatContext *inFormatContext;
    FormatContext *outFormatContext;
    QVector<TranscodeContext *> transcodeContexts{};
    AVCodecID enCodecId = AV_CODEC_ID_NONE;
    QVector<FilteringContext *> filteringContexts{};

    bool gpuDecode = false;

    AVError error;
};

Transcode::Transcode(QObject *parent)
    : QObject{parent}
    , d_ptr(new TranscodePrivate(this))
{}

Transcode::~Transcode() {}

bool Transcode::openInputFile(const QString &filepath)
{
    auto ret = d_ptr->inFormatContext->openFilePath(filepath);
    if (!ret) {
        return ret;
    }
    d_ptr->inFormatContext->findStream();
    auto stream_num = d_ptr->inFormatContext->streams();
    for (int i = 0; i < stream_num; i++) {
        QSharedPointer<AVContextInfo> contextInfoPtr;
        auto codec_type = d_ptr->inFormatContext->stream(i)->codecpar->codec_type;
        switch (codec_type) {
        case AVMEDIA_TYPE_AUDIO:
        case AVMEDIA_TYPE_VIDEO:
        case AVMEDIA_TYPE_SUBTITLE: contextInfoPtr.reset(new AVContextInfo); break;
        default: break;
        }
        if (contextInfoPtr.isNull()) {
            continue;
        }
        if (!d_ptr->setInMediaIndex(contextInfoPtr.data(), i)) {
            return false;
        }
        if (codec_type == AVMEDIA_TYPE_VIDEO || codec_type == AVMEDIA_TYPE_AUDIO) {
            contextInfoPtr->openCodec(d_ptr->gpuDecode);
        }
        auto transContext = new TranscodeContext;
        transContext->decContextInfo = contextInfoPtr;
        transContext->decFrame.reset(new Frame);
        d_ptr->transcodeContexts.append(transContext);
    }
    d_ptr->inFormatContext->dumpFormat();

    return true;
}

bool Transcode::openOutputFile(const QString &filepath)
{
    auto ret = d_ptr->outFormatContext->openFilePath(filepath, FormatContext::WriteOnly);
    if (!ret) {
        return ret;
    }
    auto stream_num = d_ptr->inFormatContext->streams();
    for (int i = 0; i < stream_num; i++) {
        auto stream = d_ptr->outFormatContext->createStream();
        if (!stream) {
            return false;
        }
        auto transContext = d_ptr->transcodeContexts.at(i);
        auto decContextInfo = transContext->decContextInfo;
        switch (decContextInfo->mediaType()) {
        case AVMEDIA_TYPE_AUDIO:
        case AVMEDIA_TYPE_VIDEO: {
            QSharedPointer<AVContextInfo> contextInfoPtr(new AVContextInfo);
            contextInfoPtr->initEncoder(d_ptr->enCodecId);
            decContextInfo->copyToCodecParameters(contextInfoPtr.data());
            if (d_ptr->outFormatContext->avFormatContext()->oformat->flags & AVFMT_GLOBALHEADER) {
                contextInfoPtr->codecCtx()->setFlags(contextInfoPtr->codecCtx()->flags()
                                                     | AV_CODEC_FLAG_GLOBAL_HEADER);
            }
            contextInfoPtr->openCodec(d_ptr->gpuDecode);
            auto ret = avcodec_parameters_from_context(stream->codecpar,
                                                       contextInfoPtr->codecCtx()->avCodecCtx());
            if (ret < 0) {
                setError(ret);
                return ret;
            }
            stream->time_base = decContextInfo->timebase();
            transContext->encContextInfo = contextInfoPtr;
        } break;
        case AVMEDIA_TYPE_UNKNOWN:
            qFatal("Elementary stream #%d is of unknown type, cannot proceed", i);
            return false;
        default: {
            auto inStream = d_ptr->inFormatContext->stream(i);
            auto ret = avcodec_parameters_copy(stream->codecpar, inStream->codecpar);
            if (ret < 0) {
                qErrnoWarning("Copying parameters for stream #%u failed", i);
                return ret;
            }
            stream->time_base = inStream->time_base;
        } break;
        }
    }

    d_ptr->outFormatContext->dumpFormat();

    d_ptr->outFormatContext->avio_open();

    return d_ptr->outFormatContext->writeHeader();
}

void Transcode::reset()
{
    d_ptr->reset();
}

void Transcode::initFilters()
{
    auto stream_num = d_ptr->inFormatContext->streams();
    for (int i = 0; i < stream_num; i++) {
        auto filteringContext = new FilteringContext;
        d_ptr->filteringContexts.append(filteringContext);
        auto codec_type = d_ptr->inFormatContext->stream(i)->codecpar->codec_type;
        if (codec_type != AVMEDIA_TYPE_AUDIO && codec_type != AVMEDIA_TYPE_VIDEO) {
            continue;
        }

        init_filter(filteringContext,
                    d_ptr->transcodeContexts.at(i)->decContextInfo.data(),
                    d_ptr->transcodeContexts.at(i)->encContextInfo.data(),
                    codec_type == AVMEDIA_TYPE_VIDEO ? "null" : "anull");
        filteringContext->encPacket.reset(new Packet);
        filteringContext->filteredFrame.reset(new Frame);
    }
}

void Transcode::setError(int errorCode)
{
    d_ptr->error.setError(errorCode);
    emit error(d_ptr->error);
}

} // namespace Ffmpeg
