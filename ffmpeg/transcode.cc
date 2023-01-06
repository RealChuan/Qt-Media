#include "transcode.hpp"
#include "avcontextinfo.h"
#include "averror.h"
#include "codeccontext.h"
#include "decoder.h"
#include "formatcontext.h"
#include "frame.hpp"
#include "packet.h"

#include <filter/filtercontext.hpp>
#include <filter/filtergraph.hpp>
#include <filter/filterinout.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavutil/channel_layout.h>
}

namespace Ffmpeg {

struct FilteringContext
{
    QSharedPointer<FilterContext> buffersinkCtx;
    QSharedPointer<FilterContext> buffersrcCtx;
    QSharedPointer<FilterGraph> filterGraph;
};

struct TranscodeContext
{
    QSharedPointer<AVContextInfo> decContextInfo;
    QSharedPointer<AVContextInfo> encContextInfo;
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
    fctx->buffersinkCtx = buffersink_ctx;
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
        inFormatContext->close();
        outFormatContext->close();
    }

    QObject *owner;

    QString inFilePath;
    QString outFilepath;
    FormatContext *inFormatContext;
    FormatContext *outFormatContext;
    QVector<TranscodeContext *> transcodeContexts{};
    QVector<FilteringContext *> filteringContexts{};
    AVCodecID audioEnCodecId = AV_CODEC_ID_NONE;
    AVCodecID videoEnCodecId = AV_CODEC_ID_NONE;
    QSize size = QSize(-1, -1);
    int quailty = -1;
    int64_t minBitrate = -1;
    int64_t maxBitrate = -1;
    int crf = 18;
    QStringList presets{"ultrafast",
                        "superfast",
                        "veryfast",
                        "faster",
                        "fast",
                        "medium",
                        "slow",
                        "slower",
                        "veryslow",
                        "placebo"};
    QString preset = "medium";
    QStringList tunes{"film",
                      "animation",
                      "grain",
                      "stillimage",
                      "psnr",
                      "ssim",
                      "fastdecode",
                      "zerolatency"};
    QString tune = "film";
    QStringList profiles{"baseline", "extended", "main", "high"};
    QString profile = "main";

    bool gpuDecode = false;

    AVError error;

    volatile bool runing = true;
};

Transcode::Transcode(QObject *parent)
    : QThread{parent}
    , d_ptr(new TranscodePrivate(this))
{}

Transcode::~Transcode()
{
    stopTranscode();
}

void Transcode::setInFilePath(const QString &filePath)
{
    d_ptr->inFilePath = filePath;
}

void Transcode::setOutFilePath(const QString &filepath)
{
    d_ptr->outFilepath = filepath;
}

void Transcode::setAudioEncodecID(AVCodecID codecID)
{
    d_ptr->audioEnCodecId = codecID;
}

void Transcode::setVideoEnCodecID(AVCodecID codecID)
{
    d_ptr->videoEnCodecId = codecID;
}

void Transcode::setSize(const QSize &size)
{
    d_ptr->size = size;
}

void Transcode::setQuailty(int quailty)
{
    d_ptr->quailty = quailty;
}

void Transcode::setMinBitrate(int64_t bitrate)
{
    d_ptr->minBitrate = bitrate;
}

void Transcode::setMaxBitrate(int64_t bitrate)
{
    d_ptr->maxBitrate = bitrate;
}

void Transcode::setCrf(int crf)
{
    d_ptr->crf = crf;
}

void Transcode::setPreset(const QString &preset)
{
    Q_ASSERT(d_ptr->presets.contains(preset));
    d_ptr->preset = preset;
}

QString Transcode::preset() const
{
    return d_ptr->preset;
}

QStringList Transcode::presets() const
{
    return d_ptr->presets;
}

void Transcode::setTune(const QString &tune)
{
    Q_ASSERT(d_ptr->tunes.contains(tune));
    d_ptr->tune = tune;
}

QString Transcode::tune() const
{
    return d_ptr->tune;
}

QStringList Transcode::tunes() const
{
    return d_ptr->tunes;
}

void Transcode::setProfile(const QString &profile)
{
    Q_ASSERT(d_ptr->profiles.contains(profile));
    d_ptr->profile = profile;
}

QString Transcode::profile() const
{
    return d_ptr->profile;
}

QStringList Transcode::profiles() const
{
    return d_ptr->profiles;
}

void Transcode::startTranscode()
{
    stopTranscode();
    d_ptr->runing = true;
    start();
}

void Transcode::stopTranscode()
{
    d_ptr->runing = false;
    if (isRunning()) {
        quit();
        wait();
    }
    d_ptr->reset();
}

bool Transcode::openInputFile()
{
    Q_ASSERT(!d_ptr->inFilePath.isEmpty());
    auto ret = d_ptr->inFormatContext->openFilePath(d_ptr->inFilePath);
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
        d_ptr->transcodeContexts.append(transContext);
    }
    d_ptr->inFormatContext->dumpFormat();

    return true;
}

bool Transcode::openOutputFile()
{
    Q_ASSERT(!d_ptr->outFilepath.isEmpty());
    auto ret = d_ptr->outFormatContext->openFilePath(d_ptr->outFilepath, FormatContext::WriteOnly);
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
        stream->codecpar->codec_type = decContextInfo->mediaType();
        switch (decContextInfo->mediaType()) {
        case AVMEDIA_TYPE_AUDIO:
        case AVMEDIA_TYPE_VIDEO: {
            QSharedPointer<AVContextInfo> contextInfoPtr(new AVContextInfo);
            contextInfoPtr->setIndex(i);
            contextInfoPtr->setStream(stream);
            contextInfoPtr->initEncoder(decContextInfo->mediaType() == AVMEDIA_TYPE_AUDIO
                                            ? d_ptr->audioEnCodecId
                                            : d_ptr->videoEnCodecId);
            //contextInfoPtr->initEncoder(decContextInfo->codecCtx()->avCodecCtx()->codec_id);
            decContextInfo->copyToCodecParameters(contextInfoPtr.data());
            contextInfoPtr->setQuailty(d_ptr->quailty);
            contextInfoPtr->setCrf(d_ptr->crf);
            contextInfoPtr->setPreset(d_ptr->preset);
            contextInfoPtr->setTune(d_ptr->tune);
            if (decContextInfo->mediaType() == AVMEDIA_TYPE_VIDEO) {
                contextInfoPtr->setSize(d_ptr->size);
                contextInfoPtr->setMinBitrate(d_ptr->minBitrate);
                contextInfoPtr->setMaxBitrate(d_ptr->maxBitrate);
                contextInfoPtr->setProfile(d_ptr->profile);
            }
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

void Transcode::run()
{
    QElapsedTimer timer;
    timer.start();
    qDebug() << "Start Transcoding";
    d_ptr->reset();
    if (!openInputFile()) {
        return;
    }
    if (!openOutputFile()) {
        return;
    }
    initFilters();
    loop();
    cleanup();
    qDebug() << "Finish Transcoding: " << timer.elapsed() / 1000.0 / 60.0;
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
        QString filter_spec;
        if (codec_type == AVMEDIA_TYPE_VIDEO) {
            if (d_ptr->size.isValid()) { // "scale=320:240"
                filter_spec = QString("scale=%1:%2")
                                  .arg(QString::number(d_ptr->size.width()),
                                       QString::number(d_ptr->size.height()));
            } else {
                filter_spec = "null";
            }
        } else {
            filter_spec = "anull";
        }
        init_filter(filteringContext,
                    d_ptr->transcodeContexts.at(i)->decContextInfo.data(),
                    d_ptr->transcodeContexts.at(i)->encContextInfo.data(),
                    filter_spec.toLocal8Bit().constData());
    }
}

void Transcode::loop()
{
    auto duration = d_ptr->inFormatContext->duration();
    while (d_ptr->runing) {
        QScopedPointer<Packet> packetPtr(new Packet);
        if (!d_ptr->inFormatContext->readFrame(packetPtr.get())) {
            break;
        }
        auto stream_index = packetPtr->avPacket()->stream_index;
        auto filter_graph = d_ptr->filteringContexts.at(stream_index)->filterGraph;
        if (filter_graph.isNull()) {
            packetPtr->rescaleTs(d_ptr->inFormatContext->stream(stream_index)->time_base,
                                 d_ptr->outFormatContext->stream(stream_index)->time_base);
            d_ptr->outFormatContext->writeFrame(packetPtr.data());
        } else {
            auto transcodeCtx = d_ptr->transcodeContexts.at(stream_index);
            packetPtr->rescaleTs(d_ptr->inFormatContext->stream(stream_index)->time_base,
                                 transcodeCtx->decContextInfo->timebase());
            auto frames = transcodeCtx->decContextInfo->decodeFrame(packetPtr.data());
            for (auto frame : frames) {
                QScopedPointer<Frame> framePtr(frame);
                filterEncodeWriteframe(frame, stream_index);
            }
        }
        Ffmpeg::calculateTime(packetPtr.data(),
                              d_ptr->transcodeContexts.at(stream_index)->decContextInfo.data());

        emit progressChanged(packetPtr->pts() * 1000 / duration);
    }
}

void Transcode::cleanup()
{
    auto stream_num = d_ptr->inFormatContext->streams();
    for (int i = 0; i < stream_num; i++) {
        if (d_ptr->filteringContexts.at(i)->filterGraph.isNull()) {
            continue;
        }
        QScopedPointer<Frame> framePtr(new Frame);
        framePtr->setAVFrameNull();
        filterEncodeWriteframe(framePtr.data(), i);
        flushEncoder(i);
    }
    d_ptr->outFormatContext->writeTrailer();
    d_ptr->reset();
}

bool Transcode::filterEncodeWriteframe(Frame *frame, uint stream_index)
{
    auto filteringCtx = d_ptr->filteringContexts.at(stream_index);
    if (!filteringCtx->buffersrcCtx->buffersrc_addFrameFlags(frame)) {
        return false;
    }
    QScopedPointer<Frame> framePtr(new Frame);
    while (filteringCtx->buffersinkCtx->buffersink_getFrame(framePtr.data())) {
        framePtr->setPictType(AV_PICTURE_TYPE_NONE);
        encodeWriteFrame(stream_index, 0, framePtr.data());
    }
    return true;
}

bool Transcode::encodeWriteFrame(uint stream_index, int flush, Frame *frame)
{
    auto transcodeCtx = d_ptr->transcodeContexts.at(stream_index);
    QVector<Packet *> packets{};
    if (flush) {
        QScopedPointer<Frame> framePtr(new Frame);
        framePtr->setAVFrameNull();
        packets = transcodeCtx->encContextInfo->encodeFrame(framePtr.data());
    } else {
        packets = transcodeCtx->encContextInfo->encodeFrame(frame);
    }
    for (auto packet : packets) {
        packet->setStreamIndex(stream_index);
        packet->rescaleTs(transcodeCtx->encContextInfo->timebase(),
                          d_ptr->outFormatContext->stream(stream_index)->time_base);
        d_ptr->outFormatContext->writeFrame(packet);
    }
    return true;
}

bool Transcode::flushEncoder(uint stream_index)
{
    auto codecCtx
        = d_ptr->transcodeContexts.at(stream_index)->encContextInfo->codecCtx()->avCodecCtx();
    if (!(codecCtx->codec->capabilities & AV_CODEC_CAP_DELAY)) {
        return true;
    }
    return encodeWriteFrame(stream_index, 1, nullptr);
}

void Transcode::setError(int errorCode)
{
    d_ptr->error.setError(errorCode);
    emit error(d_ptr->error);
}

} // namespace Ffmpeg
