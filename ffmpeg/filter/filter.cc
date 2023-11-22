#include "filter.hpp"
#include "filtercontext.hpp"
#include "filtergraph.hpp"
#include "filterinout.hpp"

#include <ffmpeg/avcontextinfo.h>
#include <ffmpeg/codeccontext.h>
#include <ffmpeg/frame.hpp>

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
}

namespace Ffmpeg {

class Filter::FilterPrivate
{
public:
    explicit FilterPrivate(Filter *q)
        : q_ptr(q)
    {
        filterGraph = new FilterGraph(q_ptr);
    }

    void initVideoFilter(Frame *frame)
    {
        auto *avCodecCtx = decContextInfo->codecCtx()->avCodecCtx();
        buffersrcCtx = new FilterContext("buffer", q_ptr);
        buffersinkCtx = new FilterContext("buffersink", q_ptr);
        auto timeBase = decContextInfo->timebase();
        auto args
            = QString::asprintf("video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                                avCodecCtx->width,
                                avCodecCtx->height,
                                frame->avFrame()->format, //dec_ctx->pix_fmt,
                                timeBase.num,
                                timeBase.den,
                                avCodecCtx->sample_aspect_ratio.num,
                                avCodecCtx->sample_aspect_ratio.den);
        qDebug() << "Video filter in args:" << args;

        create(args);
    }

    void initAudioFilter()
    {
        auto *avCodecCtx = decContextInfo->codecCtx()->avCodecCtx();
        buffersrcCtx = new FilterContext("abuffer", q_ptr);
        buffersinkCtx = new FilterContext("abuffersink", q_ptr);
        if (avCodecCtx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
            av_channel_layout_default(&avCodecCtx->ch_layout, avCodecCtx->ch_layout.nb_channels);
        }
        char buf[64];
        av_channel_layout_describe(&avCodecCtx->ch_layout, buf, sizeof(buf));
        auto args = QString::asprintf(
            "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s" PRIx64,
            1,
            avCodecCtx->sample_rate,
            avCodecCtx->sample_rate,
            av_get_sample_fmt_name(avCodecCtx->sample_fmt),
            buf);
        qDebug() << "Audio filter in args:" << args;

        create(args);
    }

    void create(const QString &args)
    {
        buffersrcCtx->create("in", args, filterGraph);
        buffersinkCtx->create("out", "", filterGraph);
    }

    void config(const QString &filterSpec)
    {
        QScopedPointer<FilterInOut> fliterOutPtr(new FilterInOut);
        QScopedPointer<FilterInOut> fliterInPtr(new FilterInOut);
        auto *outputs = fliterOutPtr->avFilterInOut();
        auto *inputs = fliterInPtr->avFilterInOut();
        /* Endpoints for the filter graph. */
        outputs->name = av_strdup("in");
        outputs->filter_ctx = buffersrcCtx->avFilterContext();
        outputs->pad_idx = 0;
        outputs->next = nullptr;

        inputs->name = av_strdup("out");
        inputs->filter_ctx = buffersinkCtx->avFilterContext();
        inputs->pad_idx = 0;
        inputs->next = nullptr;

        filterGraph->parse(filterSpec, fliterInPtr.data(), fliterOutPtr.data());
        filterGraph->config();

        //    if (!(enc_ctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)) {
        //        buffersink_ctx->buffersink_setFrameSize(enc_ctx->frame_size);
        //    }
    }

    Filter *q_ptr;

    AVContextInfo *decContextInfo;
    FilterContext *buffersrcCtx;
    FilterContext *buffersinkCtx;
    FilterGraph *filterGraph;
};

Filter::Filter(AVContextInfo *decContextInfo, QObject *parent)
    : QObject{parent}
    , d_ptr(new FilterPrivate(this))
{
    d_ptr->decContextInfo = decContextInfo;
}

Filter::~Filter() = default;

auto Filter::init(Frame *frame) -> bool
{
    switch (d_ptr->decContextInfo->mediaType()) {
    case AVMEDIA_TYPE_AUDIO: d_ptr->initAudioFilter(); break;
    case AVMEDIA_TYPE_VIDEO: d_ptr->initVideoFilter(frame); break;
    default: return false;
    }

    return true;
}

auto Filter::filterFrame(Frame *frame) -> QVector<FramePtr>
{
    QVector<FramePtr> framepPtrs{};
    if (d_ptr->buffersrcCtx->buffersrcAddFrameFlags(frame)) {
        return framepPtrs;
    }
    std::unique_ptr<Frame> framePtr(new Frame);
    while (d_ptr->buffersinkCtx->buffersinkGetFrame(framePtr.get())) {
        framePtr->setPictType(AV_PICTURE_TYPE_NONE);
        framepPtrs.emplace_back(framePtr.release());
    }
    return framepPtrs;
}

} // namespace Ffmpeg
