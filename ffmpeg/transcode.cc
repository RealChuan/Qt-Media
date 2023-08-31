#include "transcode.hpp"
#include "audiofifo.hpp"
#include "avcontextinfo.h"
#include "averrormanager.hpp"
#include "codeccontext.h"
#include "ffmpegutils.hpp"
#include "formatcontext.h"
#include "frame.hpp"
#include "packet.h"

#include <filter/filtercontext.hpp>
#include <filter/filtergraph.hpp>
#include <filter/filterinout.hpp>
#include <utils/fps.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavutil/channel_layout.h>
}

namespace Ffmpeg {

struct TranscodeContext
{
    QSharedPointer<AVContextInfo> decContextInfoPtr;
    QSharedPointer<AVContextInfo> encContextInfoPtr;

    bool initFilter = false;
    QSharedPointer<FilterContext> buffersinkCtxPtr;
    QSharedPointer<FilterContext> buffersrcCtxPtr;
    QSharedPointer<FilterGraph> filterGraphPtr;

    QSharedPointer<AudioFifo> audioFifoPtr;
    int64_t audioPts = 0;
};

bool init_filter(TranscodeContext *transcodeContext, const char *filter_spec, Frame *frame)
{
    QSharedPointer<FilterContext> buffersrc_ctx;
    QSharedPointer<FilterContext> buffersink_ctx;
    QSharedPointer<FilterGraph> filter_graph(new FilterGraph);
    auto dec_ctx = transcodeContext->decContextInfoPtr->codecCtx()->avCodecCtx();
    auto enc_ctx = transcodeContext->encContextInfoPtr->codecCtx()->avCodecCtx();
    switch (transcodeContext->decContextInfoPtr->mediaType()) {
    case AVMEDIA_TYPE_VIDEO: {
        buffersrc_ctx.reset(new FilterContext("buffer"));
        buffersink_ctx.reset(new FilterContext("buffersink"));
        auto time_base = transcodeContext->decContextInfoPtr->stream()->time_base;
        auto args
            = QString::asprintf("video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                                dec_ctx->width,
                                dec_ctx->height,
                                frame->avFrame()->format, //dec_ctx->pix_fmt,
                                time_base.num,
                                time_base.den,
                                dec_ctx->sample_aspect_ratio.num,
                                dec_ctx->sample_aspect_ratio.den);
        qInfo() << "Video filter in args:" << args;
        buffersrc_ctx->create("in", args, filter_graph.data());
        buffersink_ctx->create("out", "", filter_graph.data());
        auto pix_fmt = transcodeContext->encContextInfoPtr->pixfmt();
        av_opt_set_bin(buffersink_ctx->avFilterContext(),
                       "pix_fmts",
                       (uint8_t *) &pix_fmt,
                       sizeof(pix_fmt),
                       AV_OPT_SEARCH_CHILDREN);
        //        av_opt_set_bin(buffersink_ctx->avFilterContext(),
        //                       "pix_fmts",
        //                       (uint8_t *) &enc_ctx->pix_fmt,
        //                       sizeof(enc_ctx->pix_fmt),
        //                       AV_OPT_SEARCH_CHILDREN);
    } break;
    case AVMEDIA_TYPE_AUDIO: {
        buffersrc_ctx.reset(new FilterContext("abuffer"));
        buffersink_ctx.reset(new FilterContext("abuffersink"));
        if (!dec_ctx->channel_layout) {
            dec_ctx->channel_layout = av_get_default_channel_layout(dec_ctx->channels);
        }
        auto args = QString::asprintf(
            "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
            1,
            dec_ctx->sample_rate,
            dec_ctx->sample_rate,
            av_get_sample_fmt_name(dec_ctx->sample_fmt),
            dec_ctx->channel_layout);
        qInfo() << "Audio filter in args:" << args;
        buffersrc_ctx->create("in", args, filter_graph.data());
        buffersink_ctx->create("out", "", filter_graph.data());
        av_opt_set_bin(buffersink_ctx->avFilterContext(),
                       "sample_rates",
                       (uint8_t *) &enc_ctx->sample_rate,
                       sizeof(enc_ctx->sample_rate),
                       AV_OPT_SEARCH_CHILDREN);
        av_opt_set_bin(buffersink_ctx->avFilterContext(),
                       "sample_fmts",
                       (uint8_t *) &enc_ctx->sample_fmt,
                       sizeof(enc_ctx->sample_fmt),
                       AV_OPT_SEARCH_CHILDREN);
        av_opt_set_bin(buffersink_ctx->avFilterContext(),
                       "channel_layouts",
                       (uint8_t *) &enc_ctx->channel_layout,
                       sizeof(enc_ctx->channel_layout),
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

    //    if (!(enc_ctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)) {
    //        buffersink_ctx->buffersink_setFrameSize(enc_ctx->frame_size);
    //    }

    transcodeContext->buffersrcCtxPtr = buffersrc_ctx;
    transcodeContext->buffersinkCtxPtr = buffersink_ctx;
    transcodeContext->filterGraphPtr = filter_graph;

    return true;
}

class Transcode::TranscodePrivate
{
public:
    TranscodePrivate(QObject *parent)
        : owner(parent)
        , inFormatContext(new FormatContext(owner))
        , outFormatContext(new FormatContext(owner))
        , fpsPtr(new Utils::Fps)
    {}

    ~TranscodePrivate() { reset(); }

    bool openInputFile()
    {
        Q_ASSERT(!inFilePath.isEmpty());
        auto ret = inFormatContext->openFilePath(inFilePath);
        if (!ret) {
            return ret;
        }
        inFormatContext->findStream();
        auto stream_num = inFormatContext->streams();
        for (int i = 0; i < stream_num; i++) {
            auto transContext = new TranscodeContext;
            transcodeContexts.append(transContext);
            auto stream = inFormatContext->stream(i);
            auto codec_type = stream->codecpar->codec_type;
            if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                continue;
            }
            QSharedPointer<AVContextInfo> contextInfoPtr;
            switch (codec_type) {
            case AVMEDIA_TYPE_AUDIO:
            case AVMEDIA_TYPE_VIDEO:
            case AVMEDIA_TYPE_SUBTITLE: contextInfoPtr.reset(new AVContextInfo); break;
            default: return false;
            }
            if (!setInMediaIndex(contextInfoPtr.data(), i)) {
                return false;
            }
            if (codec_type == AVMEDIA_TYPE_VIDEO || codec_type == AVMEDIA_TYPE_AUDIO) {
                contextInfoPtr->openCodec(useGpuDecode ? AVContextInfo::GpuDecode
                                                       : AVContextInfo::NotUseGpu);
            }
            transContext->decContextInfoPtr = contextInfoPtr;
        }
        inFormatContext->dumpFormat();
        return true;
    }

    bool openOutputFile()
    {
        Q_ASSERT(!outFilepath.isEmpty());
        auto ret = outFormatContext->openFilePath(outFilepath, FormatContext::WriteOnly);
        if (!ret) {
            return ret;
        }
        outFormatContext->copyChapterFrom(inFormatContext);
        auto stream_num = inFormatContext->streams();
        for (int i = 0; i < stream_num; i++) {
            auto inStream = inFormatContext->stream(i);
            auto stream = outFormatContext->createStream();
            if (!stream) {
                return false;
            }
            av_dict_copy(&stream->metadata, inStream->metadata, 0);
            stream->disposition = inStream->disposition;
            stream->discard = inStream->discard;
            stream->sample_aspect_ratio = inStream->sample_aspect_ratio;
            stream->avg_frame_rate = inStream->avg_frame_rate;
            stream->event_flags = inStream->event_flags;
            auto transContext = transcodeContexts.at(i);
            auto decContextInfo = transContext->decContextInfoPtr;
            if (inStream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                auto ret = avcodec_parameters_copy(stream->codecpar, inStream->codecpar);
                if (ret < 0) {
                    qErrnoWarning("Copying parameters for stream #%u failed", i);
                    return ret;
                }
                stream->time_base = inStream->time_base;
                stream->codecpar->width = inStream->codecpar->width > 0 ? inStream->codecpar->width
                                                                        : size.width();
                stream->codecpar->height = inStream->codecpar->height > 0
                                               ? inStream->codecpar->height
                                               : size.height();
                continue;
            }
            stream->codecpar->codec_type = decContextInfo->mediaType();
            switch (decContextInfo->mediaType()) {
            case AVMEDIA_TYPE_AUDIO:
            case AVMEDIA_TYPE_VIDEO: {
                QSharedPointer<AVContextInfo> contextInfoPtr(new AVContextInfo);
                contextInfoPtr->setIndex(i);
                contextInfoPtr->setStream(stream);
                contextInfoPtr->initEncoder(decContextInfo->mediaType() == AVMEDIA_TYPE_AUDIO
                                                ? audioEncoderName
                                                : videoEncoderName);
                //contextInfoPtr->initEncoder(decContextInfo->codecCtx()->avCodecCtx()->codec_id);
                auto codecCtx = contextInfoPtr->codecCtx();
                auto avCodecCtx = codecCtx->avCodecCtx();
                decContextInfo->codecCtx()->copyToCodecParameters(codecCtx);
                // ffmpeg example transcoding.c ? framerate, sample_rate
                codecCtx->avCodecCtx()->time_base = decContextInfo->timebase();
                codecCtx->setQuailty(quailty);
                codecCtx->setCrf(crf);
                codecCtx->setPreset(preset);
                codecCtx->setTune(tune);
                if (decContextInfo->mediaType() == AVMEDIA_TYPE_VIDEO) {
                    codecCtx->setSize(size);
                    codecCtx->setMinBitrate(minBitrate);
                    codecCtx->setMaxBitrate(maxBitrate);
                    codecCtx->setProfile(profile);
                }
                if (outFormatContext->avFormatContext()->oformat->flags & AVFMT_GLOBALHEADER) {
                    avCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
                }
                contextInfoPtr->openCodec(AVContextInfo::GpuEncode);
                auto ret = avcodec_parameters_from_context(stream->codecpar,
                                                           contextInfoPtr->codecCtx()->avCodecCtx());
                if (ret < 0) {
                    SET_ERROR_CODE(ret);
                    return ret;
                }
                stream->time_base = decContextInfo->timebase();
                transContext->encContextInfoPtr = contextInfoPtr;
            } break;
            case AVMEDIA_TYPE_UNKNOWN:
                qFatal("Elementary stream #%d is of unknown type, cannot proceed", i);
                return false;
            default: {
                auto ret = avcodec_parameters_copy(stream->codecpar, inStream->codecpar);
                if (ret < 0) {
                    SET_ERROR_CODE(ret);
                    return ret;
                }
                stream->time_base = inStream->time_base;
            } break;
            }
        }
        outFormatContext->dumpFormat();
        outFormatContext->avio_open();
        return outFormatContext->writeHeader();
    }

    void initFilters(int stream_index, Frame *frame)
    {
        auto transcodeCtx = transcodeContexts.at(stream_index);
        if (transcodeCtx->decContextInfoPtr.isNull()) {
            return;
        }
        auto codec_type = inFormatContext->stream(stream_index)->codecpar->codec_type;
        if (codec_type != AVMEDIA_TYPE_AUDIO && codec_type != AVMEDIA_TYPE_VIDEO) {
            return;
        }
        QString filter_spec;
        if (codec_type == AVMEDIA_TYPE_VIDEO) {
            if (size.isValid()) { // "scale=320:240"
                filter_spec = QString("scale=%1:%2")
                                  .arg(QString::number(size.width()),
                                       QString::number(size.height()));
            }
            if (!subtitleFilename.isEmpty()) {
                // "subtitles=filename=..." burn subtitle into video
                if (!filter_spec.isEmpty()) {
                    filter_spec += ",";
                }
                filter_spec += QString("subtitles=filename='%1':original_size=%2x%3")
                                   .arg(subtitleFilename,
                                        QString::number(size.width()),
                                        QString::number(size.height()));
            }
            if (filter_spec.isEmpty()) {
                filter_spec = "null";
            }
            qInfo() << "Video Filter: " << filter_spec;
        } else {
            filter_spec = "anull";
        }
        init_filter(transcodeCtx, filter_spec.toLocal8Bit().constData(), frame);
    }

    void initAudioFifo()
    {
        auto stream_num = inFormatContext->streams();
        for (int i = 0; i < stream_num; i++) {
            auto transCtx = transcodeContexts.at(i);
            if (transCtx->decContextInfoPtr.isNull()) {
                continue;
            }
            if (transCtx->decContextInfoPtr->mediaType() == AVMEDIA_TYPE_AUDIO) {
                transCtx->audioFifoPtr.reset(new AudioFifo(transCtx->encContextInfoPtr->codecCtx()));
            }
        }
    }

    void cleanup()
    {
        auto stream_num = inFormatContext->streams();
        for (int i = 0; i < stream_num; i++) {
            if (transcodeContexts.at(i)->filterGraphPtr.isNull()) {
                continue;
            }
            if (!transcodeContexts.at(i)->audioFifoPtr.isNull()) {
                fliterAudioFifo(i, nullptr, true);
            }
            QScopedPointer<Frame> framePtr(new Frame);
            framePtr->destroyFrame();
            filterEncodeWriteframe(framePtr.data(), i);
            flushEncoder(i);
        }
        outFormatContext->writeTrailer();
        reset();
    }

    bool filterEncodeWriteframe(Frame *frame, uint stream_index)
    {
        auto transcodeCtx = transcodeContexts.at(stream_index);
        if (!transcodeCtx->buffersrcCtxPtr->buffersrc_addFrameFlags(frame)) {
            return false;
        }
        QSharedPointer<Frame> framePtr(new Frame);
        while (transcodeCtx->buffersinkCtxPtr->buffersink_getFrame(framePtr.data())) {
            framePtr->setPictType(AV_PICTURE_TYPE_NONE);
            if (transcodeCtx->audioFifoPtr.isNull()) {
                encodeWriteFrame(stream_index, 0, framePtr);
            } else {
                fliterAudioFifo(stream_index, framePtr);
            }
        }
        return true;
    }

    void fliterAudioFifo(uint stream_index, QSharedPointer<Frame> framePtr, bool finish = false)
    {
        //qDebug() << "old: " << stream_index << frame->avFrame()->pts;
        if (!framePtr.isNull()) {
            addSamplesToFifo(framePtr.data(), stream_index);
        }
        while (1) {
            auto framePtr = takeSamplesFromFifo(stream_index, finish);
            if (framePtr.isNull()) {
                return;
            }
            encodeWriteFrame(stream_index, 0, framePtr);
        }
    }

    bool addSamplesToFifo(Frame *frame, uint stream_index)
    {
        auto transcodeCtx = transcodeContexts.at(stream_index);
        auto audioFifoPtr = transcodeCtx->audioFifoPtr;
        if (audioFifoPtr.isNull()) {
            return false;
        }
        const int output_frame_size
            = transcodeCtx->encContextInfoPtr->codecCtx()->avCodecCtx()->frame_size;
        if (audioFifoPtr->size() >= output_frame_size) {
            return true;
        }
        if (!audioFifoPtr->realloc(audioFifoPtr->size() + frame->avFrame()->nb_samples)) {
            return false;
        }
        return audioFifoPtr->write((void **) frame->avFrame()->data, frame->avFrame()->nb_samples);
    }

    QSharedPointer<Frame> takeSamplesFromFifo(uint stream_index, bool finished = false)
    {
        auto transcodeCtx = transcodeContexts.at(stream_index);
        auto audioFifoPtr = transcodeCtx->audioFifoPtr;
        if (audioFifoPtr.isNull()) {
            return nullptr;
        }
        auto enc_ctx = transcodeCtx->encContextInfoPtr->codecCtx()->avCodecCtx();
        if (audioFifoPtr->size() < enc_ctx->frame_size && !finished) {
            return nullptr;
        }
        const int frame_size = FFMIN(audioFifoPtr->size(), enc_ctx->frame_size);
        QSharedPointer<Frame> framePtr(new Frame);
        auto frame = framePtr->avFrame();
        frame->nb_samples = frame_size;
        frame->channel_layout = enc_ctx->channel_layout;
        frame->format = enc_ctx->sample_fmt;
        frame->sample_rate = enc_ctx->sample_rate;
        if (!framePtr->getBuffer()) {
            return nullptr;
        }
        if (!audioFifoPtr->read((void **) framePtr->avFrame()->data, frame_size)) {
            return nullptr;
        }
        // fix me?
        frame->pts = transcodeCtx->audioPts / av_q2d(transcodeCtx->decContextInfoPtr->timebase())
                     / transcodeCtx->decContextInfoPtr->codecCtx()->avCodecCtx()->sample_rate;
        transcodeCtx->audioPts += frame->nb_samples;
        //qDebug() << "new: " << stream_index << frame->pts;

        return framePtr;
    }

    bool encodeWriteFrame(uint stream_index, int flush, QSharedPointer<Frame> framePtr)
    {
        auto transcodeCtx = transcodeContexts.at(stream_index);
        std::vector<PacketPtr> packetPtrs{};
        if (flush) {
            QSharedPointer<Frame> frame_tmp_ptr(new Frame);
            frame_tmp_ptr->destroyFrame();
            packetPtrs = transcodeCtx->encContextInfoPtr->encodeFrame(frame_tmp_ptr);
        } else {
            packetPtrs = transcodeCtx->encContextInfoPtr->encodeFrame(framePtr);
        }
        for (const auto &packetPtr : packetPtrs) {
            packetPtr->setStreamIndex(stream_index);
            packetPtr->rescaleTs(transcodeCtx->encContextInfoPtr->timebase(),
                                 outFormatContext->stream(stream_index)->time_base);
            outFormatContext->writePacket(packetPtr.data());
        }
        return true;
    }

    bool flushEncoder(uint stream_index)
    {
        auto codecCtx
            = transcodeContexts.at(stream_index)->encContextInfoPtr->codecCtx()->avCodecCtx();
        if (!(codecCtx->codec->capabilities & AV_CODEC_CAP_DELAY)) {
            return true;
        }
        return encodeWriteFrame(stream_index, 1, nullptr);
    }

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
        inFormatContext->close();
        outFormatContext->close();

        subtitleFilename.clear();

        fpsPtr->reset();
    }

    QObject *owner;

    QString inFilePath;
    QString outFilepath;
    FormatContext *inFormatContext;
    FormatContext *outFormatContext;
    QVector<TranscodeContext *> transcodeContexts{};
    QString audioEncoderName;
    QString videoEncoderName;
    QSize size = QSize(-1, -1);
    QString subtitleFilename;
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

    bool useGpuDecode = false;

    std::atomic_bool runing = true;
    QScopedPointer<Utils::Fps> fpsPtr;
};

Transcode::Transcode(QObject *parent)
    : QThread{parent}
    , d_ptr(new TranscodePrivate(this))
{
    av_log_set_level(AV_LOG_INFO);

    buildConnect();
}

Transcode::~Transcode()
{
    stopTranscode();
}

void Transcode::setUseGpuDecode(bool useGpu)
{
    d_ptr->useGpuDecode = useGpu;
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
    d_ptr->audioEncoderName = avcodec_get_name(codecID);
}

void Transcode::setVideoEnCodecID(AVCodecID codecID)
{
    d_ptr->videoEncoderName = avcodec_get_name(codecID);
}

void Transcode::setAudioEncodecName(const QString &name)
{
    d_ptr->audioEncoderName = name;
}

void Transcode::setVideoEncodecName(const QString &name)
{
    d_ptr->videoEncoderName = name;
}

void Transcode::setSize(const QSize &size)
{
    d_ptr->size = size;
}

void Transcode::setSubtitleFilename(const QString &filename)
{
    Q_ASSERT(QFile::exists(filename));
    d_ptr->subtitleFilename = filename;
    // only for windwos
    d_ptr->subtitleFilename.replace('/', "\\\\");
    auto index = d_ptr->subtitleFilename.indexOf(":\\");
    if (index > 0) {
        d_ptr->subtitleFilename.insert(index, char('\\'));
    }
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

float Transcode::fps()
{
    return d_ptr->fpsPtr->getFps();
}

void Transcode::run()
{
    QElapsedTimer timer;
    timer.start();
    qDebug() << "Start Transcoding";
    d_ptr->reset();
    if (!d_ptr->openInputFile()) {
        qWarning() << "Open input file failed!";
        return;
    }
    if (!d_ptr->openOutputFile()) {
        qWarning() << "Open ouput file failed!";
        return;
    }
    d_ptr->initAudioFifo();
    loop();
    d_ptr->cleanup();
    qDebug() << "Finish Transcoding: " << timer.elapsed() / 1000.0 / 60.0;
}

void Transcode::buildConnect()
{
    connect(AVErrorManager::instance(), &AVErrorManager::error, this, &Transcode::error);
}

void Transcode::loop()
{
    auto duration = d_ptr->inFormatContext->duration();
    while (d_ptr->runing) {
        PacketPtr packetPtr(new Packet);
        if (!d_ptr->inFormatContext->readFrame(packetPtr.get())) {
            break;
        }
        auto stream_index = packetPtr->streamIndex();
        auto transcodeCtx = d_ptr->transcodeContexts.at(stream_index);
        auto encContextInfoPtr = transcodeCtx->encContextInfoPtr;
        if (encContextInfoPtr.isNull()) {
            packetPtr->rescaleTs(d_ptr->inFormatContext->stream(stream_index)->time_base,
                                 d_ptr->outFormatContext->stream(stream_index)->time_base);
            d_ptr->outFormatContext->writePacket(packetPtr.data());
        } else {
            packetPtr->rescaleTs(d_ptr->inFormatContext->stream(stream_index)->time_base,
                                 transcodeCtx->decContextInfoPtr->timebase());
            auto framePtrs = transcodeCtx->decContextInfoPtr->decodeFrame(packetPtr);
            for (const auto &framePtr : framePtrs) {
                if (!transcodeCtx->initFilter) {
                    d_ptr->initFilters(stream_index, framePtr.data());
                    transcodeCtx->initFilter = true;
                }
                d_ptr->filterEncodeWriteframe(framePtr.data(), stream_index);
            }

            calculatePts(packetPtr.data(),
                         d_ptr->transcodeContexts.at(stream_index)->decContextInfoPtr.data());
            emit progressChanged(packetPtr->pts() / duration);
            if (transcodeCtx->decContextInfoPtr->mediaType() == AVMEDIA_TYPE_VIDEO) {
                d_ptr->fpsPtr->update();
            }
        }
    }
}

} // namespace Ffmpeg
