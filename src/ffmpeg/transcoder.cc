#include "transcoder.hpp"
#include "audiofifo.hpp"
#include "audioframeconverter.h"
#include "avcontextinfo.h"
#include "averrormanager.hpp"
#include "codeccontext.h"
#include "encodecontext.hpp"
#include "ffmpegutils.hpp"
#include "formatcontext.h"
#include "packet.hpp"
#include "previewtask.hpp"
#include "transcodercontext.hpp"

#include <event/errorevent.hpp>
#include <event/trackevent.hpp>
#include <event/valueevent.hpp>
#include <filter/filter.hpp>
#include <filter/filtercontext.hpp>
#include <utils/concurrentqueue.hpp>
#include <utils/fps.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavutil/channel_layout.h>
}

namespace Ffmpeg {

static void copyStreamInfo(AVStream *dst, const AVStream *src)
{
    av_dict_copy(&dst->metadata, src->metadata, 0);
    dst->disposition = src->disposition;
    dst->discard = src->discard;
    dst->sample_aspect_ratio = src->sample_aspect_ratio;
    dst->avg_frame_rate = src->avg_frame_rate;
    dst->event_flags = src->event_flags;
}

static auto addSamplesToFifo(Ffmpeg::TranscoderContext *transcodeCtx, const FramePtr &framePtr)
    -> bool
{
    auto audioFifoPtr = transcodeCtx->audioFifoPtr;
    if (audioFifoPtr.isNull()) {
        return false;
    }
    const int output_frame_size
        = transcodeCtx->encContextInfoPtr->codecCtx()->avCodecCtx()->frame_size;
    auto *avFrame = framePtr->avFrame();
    if (audioFifoPtr->size() >= output_frame_size) {
        return true;
    }
    if (!audioFifoPtr->realloc(audioFifoPtr->size() + avFrame->nb_samples)) {
        return false;
    }
    return audioFifoPtr->write(reinterpret_cast<void **>(avFrame->data), avFrame->nb_samples);
}

static auto takeSamplesFromFifo(Ffmpeg::TranscoderContext *transcodeCtx, bool finished = false)
    -> FramePtr
{
    auto audioFifoPtr = transcodeCtx->audioFifoPtr;
    if (audioFifoPtr.isNull()) {
        return nullptr;
    }
    auto *enc_ctx = transcodeCtx->encContextInfoPtr->codecCtx()->avCodecCtx();
    if (audioFifoPtr->size() < enc_ctx->frame_size && !finished) {
        return nullptr;
    }
    const int frame_size = FFMIN(audioFifoPtr->size(), enc_ctx->frame_size);
    FramePtr framePtr(new Frame);
    auto *avFrame = framePtr->avFrame();
    avFrame->nb_samples = frame_size;
    av_channel_layout_copy(&avFrame->ch_layout, &enc_ctx->ch_layout);
    avFrame->format = enc_ctx->sample_fmt;
    avFrame->sample_rate = enc_ctx->sample_rate;
    if (!framePtr->getBuffer()) {
        return nullptr;
    }
    if (!audioFifoPtr->read(reinterpret_cast<void **>(framePtr->avFrame()->data), frame_size)) {
        return nullptr;
    }
    // fix me?
    avFrame->pts = transcodeCtx->audioPts / av_q2d(transcodeCtx->decContextInfoPtr->timebase())
                   / transcodeCtx->decContextInfoPtr->codecCtx()->avCodecCtx()->sample_rate;
    transcodeCtx->audioPts += avFrame->nb_samples;
    //qDebug() << "new: " << stream_index << frame->pts;

    return framePtr;
}

class Transcoder::TranscoderPrivate
{
public:
    explicit TranscoderPrivate(Transcoder *q)
        : q_ptr(q)
        , inFormatContext(new FormatContext(q_ptr))
        , outFormatContext(new FormatContext(q_ptr))
        , fpsPtr(new Utils::Fps)
    {
        threadPool = new QThreadPool(q_ptr);
        threadPool->setMaxThreadCount(2);

        QObject::connect(AVErrorManager::instance(),
                         &AVErrorManager::error,
                         q_ptr,
                         [this](const AVError &error) {
                             addPropertyChangeEvent(new AVErrorEvent(error));
                         });
    }

    ~TranscoderPrivate() { reset(); }

    auto openInputFile(bool eventChanged) -> bool
    {
        Q_ASSERT(!inFilePath.isEmpty());
        auto ret = inFormatContext->openFilePath(inFilePath);
        if (!ret) {
            return ret;
        }
        decodeContexts.clear();
        inFormatContext->findStream();
        auto stream_num = inFormatContext->streams();
        for (int i = 0; i < stream_num; i++) {
            auto *transContext = new TranscoderContext;
            transContext->filterPtr.reset(new Filter);
            transcodeContexts.append(transContext);
            auto *stream = inFormatContext->stream(i);
            auto codec_type = stream->codecpar->codec_type;
            if ((stream->disposition & AV_DISPOSITION_ATTACHED_PIC) != 0) {
                continue;
            }
            QSharedPointer<AVContextInfo> contextInfoPtr;
            switch (codec_type) {
            case AVMEDIA_TYPE_AUDIO:
            case AVMEDIA_TYPE_VIDEO:
            case AVMEDIA_TYPE_SUBTITLE: contextInfoPtr.reset(new AVContextInfo); break;
            default:
                qWarning() << "Unsupported codec type: " << av_get_media_type_string(codec_type);
                return false;
            }
            if (!setInMediaIndex(contextInfoPtr.data(), i)) {
                return false;
            }
            if (codec_type == AVMEDIA_TYPE_VIDEO || codec_type == AVMEDIA_TYPE_AUDIO) {
                if (!contextInfoPtr->openCodec(gpuDecode ? AVContextInfo::GpuDecode
                                                         : AVContextInfo::NotUseGpu)) {
                    return false;
                }
            }
            transContext->decContextInfoPtr = contextInfoPtr;
            decodeContexts.append(EncodeContext{stream, contextInfoPtr.data()});
        }
        inFormatContext->dumpFormat();

        range = {0, inFormatContext->duration()};

        if (eventChanged) {
            auto tracks = inFormatContext->audioTracks();
            tracks.append(inFormatContext->videoTracks());
            tracks.append(inFormatContext->subtitleTracks());
            tracks.append(inFormatContext->attachmentTracks());
            addPropertyChangeEvent(new MediaTrackEvent(tracks));
            addPropertyChangeEvent(new DurationEvent(inFormatContext->duration()));
        }
        return true;
    }

    auto openOutputFile() const -> bool
    {
        Q_ASSERT(!outFilepath.isEmpty());
        auto ret = outFormatContext->openFilePath(outFilepath, FormatContext::WriteOnly);
        if (!ret) {
            return ret;
        }
        outFormatContext->copyChapterFrom(inFormatContext);
        auto stream_num = inFormatContext->streams();
        int outStreamIndex = 0;
        for (int i = 0; i < stream_num; i++) {
            auto encodeContext = encodeContexts.at(i);

            auto *transContext = transcodeContexts.at(i);
            transContext->vaild = (encodeContext.streamIndex >= 0);
            if (!transContext->vaild) {
                continue;
            }
            transContext->outStreamIndex = outStreamIndex;
            outStreamIndex++;

            auto *inStream = inFormatContext->stream(i);
            auto *stream = outFormatContext->createStream();
            if (stream == nullptr) {
                return false;
            }
            copyStreamInfo(stream, inStream);

            auto decContextInfo = transContext->decContextInfoPtr;
            if ((inStream->disposition & AV_DISPOSITION_ATTACHED_PIC) != 0) {
                auto ret = avcodec_parameters_copy(stream->codecpar, inStream->codecpar);
                if (ret < 0) {
                    qErrnoWarning("Copying parameters for stream #%u failed", i);
                    return ret != 0;
                }
                stream->time_base = inStream->time_base;
                stream->codecpar->width = inStream->codecpar->width > 0
                                              ? inStream->codecpar->width
                                              : encodeContext.size.width();
                stream->codecpar->height = inStream->codecpar->height > 0
                                               ? inStream->codecpar->height
                                               : encodeContext.size.width();
                continue;
            }
            stream->codecpar->codec_type = decContextInfo->mediaType();
            switch (decContextInfo->mediaType()) {
            case AVMEDIA_TYPE_AUDIO:
            case AVMEDIA_TYPE_VIDEO: {
                QSharedPointer<AVContextInfo> contextInfoPtr(new AVContextInfo);
                contextInfoPtr->setIndex(transContext->outStreamIndex);
                contextInfoPtr->setStream(stream);
                contextInfoPtr->initEncoder(encodeContext.codecInfo().name);
                auto *codecCtx = contextInfoPtr->codecCtx();
                auto *avCodecCtx = codecCtx->avCodecCtx();
                decContextInfo->codecCtx()->copyToCodecParameters(codecCtx);
                // ffmpeg example transcoding.c ? framerate, sample_rate
                codecCtx->avCodecCtx()->time_base = decContextInfo->timebase();
                codecCtx->setEncodeParameters(encodeContext);
                if ((outFormatContext->avFormatContext()->oformat->flags & AVFMT_GLOBALHEADER)
                    != 0) {
                    avCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
                }
                if (!contextInfoPtr->openCodec(AVContextInfo::GpuEncode)) {
                    return false;
                }
                auto ret = avcodec_parameters_from_context(stream->codecpar,
                                                           contextInfoPtr->codecCtx()->avCodecCtx());
                if (ret < 0) {
                    SET_ERROR_CODE(ret);
                    return ret != 0;
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
                    return ret != 0;
                }
                stream->time_base = inStream->time_base;
            } break;
            }
        }
        outFormatContext->dumpFormat();
        if (!outFormatContext->avioOpen()) {
            return false;
        }
        return outFormatContext->writeHeader();
    }

    void initFilters(int inStreamIndex, const FramePtr &framePtr)
    {
        auto *transcodeCtx = transcodeContexts.at(inStreamIndex);
        if (transcodeCtx->decContextInfoPtr.isNull()) {
            return;
        }
        auto codec_type = inFormatContext->stream(inStreamIndex)->codecpar->codec_type;
        if (codec_type != AVMEDIA_TYPE_AUDIO && codec_type != AVMEDIA_TYPE_VIDEO) {
            return;
        }
        QString filter_spec;
        switch (codec_type) {
        case AVMEDIA_TYPE_VIDEO: {
            auto encodeContext = encodeContexts.at(inStreamIndex);
            auto original_size = encodeContext.size;
            if (original_size.isValid()) { // "scale=320:240"
                filter_spec = QString("scale=%1:%2")
                                  .arg(QString::number(original_size.width()),
                                       QString::number(original_size.height()));
            }
            if (!subtitleFilename.isEmpty()) {
                // "subtitles=filename=..." burn subtitle into video
                if (!filter_spec.isEmpty()) {
                    filter_spec += ",";
                }
                filter_spec += QString("subtitles=filename='%1':original_size=%2x%3")
                                   .arg(subtitleFilename,
                                        QString::number(original_size.width()),
                                        QString::number(original_size.height()));
            }
            if (filter_spec.isEmpty()) {
                filter_spec = "null";
            }
            qInfo() << "Video Filter: " << filter_spec;
        } break;
        default: filter_spec = "anull"; break;
        }
        transcodeCtx->initFilter(filter_spec, framePtr);
    }

    void initAudioFifo() const
    {
        for (int i = 0; i < inFormatContext->streams(); i++) {
            auto *transCtx = transcodeContexts.at(i);
            if (!transCtx->vaild || transCtx->decContextInfoPtr.isNull()) {
                continue;
            }
            if (transCtx->decContextInfoPtr->mediaType() == AVMEDIA_TYPE_AUDIO) {
                transCtx->audioFifoPtr.reset(new AudioFifo(transCtx->encContextInfoPtr->codecCtx()));
            }
        }
    }

    void cleanup()
    {
        for (int i = 0; i < transcodeContexts.size(); i++) {
            auto *transCtx = transcodeContexts.at(i);
            if (!transCtx->vaild || !transCtx->filterPtr->isInitialized()) {
                continue;
            }
            if (!transCtx->audioFifoPtr.isNull()) {
                fliterAudioFifo(transCtx, nullptr, true);
            }
            FramePtr framePtr(new Frame);
            framePtr->destroyFrame();
            filterEncodeWriteframe(framePtr, i);
            flushEncoder(transCtx);
        }
        outFormatContext->writeTrailer();
        reset();
    }

    auto filterEncodeWriteframe(const FramePtr &framePtr, int inStreamIndex) const -> bool
    {
        auto *transcodeCtx = transcodeContexts.at(inStreamIndex);
        auto framePtrs = transcodeCtx->filterPtr->filterFrame(framePtr);
        for (const auto &framePtr : std::as_const(framePtrs)) {
            framePtr->setPictType(AV_PICTURE_TYPE_NONE);
            if (transcodeCtx->audioFifoPtr.isNull()) {
                encodeWriteFrame(transcodeCtx, framePtr, false);
            } else {
                fliterAudioFifo(transcodeCtx, framePtr);
            }
        }
        return true;
    }

    void fliterAudioFifo(Ffmpeg::TranscoderContext *transcodeCtx,
                         const FramePtr &framePtr,
                         bool finish = false) const
    {
        //qDebug() << "old: " << stream_index << frame->avFrame()->pts;
        if (nullptr != framePtr) {
            addSamplesToFifo(transcodeCtx, framePtr);
        }
        while (1) {
            auto framePtr = takeSamplesFromFifo(transcodeCtx, finish);
            if (nullptr == framePtr) {
                return;
            }
            encodeWriteFrame(transcodeCtx, framePtr, false);
        }
    }

    auto encodeWriteFrame(Ffmpeg::TranscoderContext *transcodeCtx,
                          const FramePtr &framePtr,
                          bool flush) const -> bool
    {
        PacketPtrList packetPtrs{};
        if (flush) {
            FramePtr frame_tmp_ptr(new Frame);
            frame_tmp_ptr->destroyFrame();
            packetPtrs = transcodeCtx->encContextInfoPtr->encodeFrame(frame_tmp_ptr);
        } else {
            packetPtrs = transcodeCtx->encContextInfoPtr->encodeFrame(framePtr);
        }
        auto outStreamIndex = transcodeCtx->outStreamIndex;
        for (const auto &packetPtr : std::as_const(packetPtrs)) {
            packetPtr->setStreamIndex(outStreamIndex);
            packetPtr->rescaleTs(transcodeCtx->encContextInfoPtr->timebase(),
                                 outFormatContext->stream(outStreamIndex)->time_base);
            outFormatContext->writePacket(packetPtr);
        }
        return true;
    }

    auto flushEncoder(Ffmpeg::TranscoderContext *transcodeCtx) const -> bool
    {
        auto *codecCtx = transcodeCtx->encContextInfoPtr->codecCtx()->avCodecCtx();
        if ((codecCtx->codec->capabilities & AV_CODEC_CAP_DELAY) == 0) {
            return true;
        }
        return encodeWriteFrame(transcodeCtx, nullptr, true);
    }

    auto setInMediaIndex(AVContextInfo *contextInfo, int index) const -> bool
    {
        contextInfo->setIndex(index);
        contextInfo->setStream(inFormatContext->stream(index));
        return contextInfo->initDecoder(inFormatContext->guessFrameRate(index));
    }

    void loop()
    {
        while (runing.load()) {
            PacketPtr packetPtr(new Packet);
            if (!inFormatContext->readFrame(packetPtr)) {
                break;
            }
            auto stream_index = packetPtr->streamIndex();
            auto *transcodeCtx = transcodeContexts.at(stream_index);
            if (!transcodeCtx->vaild) {
                continue;
            }

            auto decContextInfoPtr = transcodeCtx->decContextInfoPtr;
            auto encContextInfoPtr = transcodeCtx->encContextInfoPtr;
            auto inTimebase = inFormatContext->stream(stream_index)->time_base;
            auto outIndex = transcodeCtx->outStreamIndex;
            if (encContextInfoPtr.isNull()) {
                packetPtr->rescaleTs(inTimebase, outFormatContext->stream(outIndex)->time_base);
                packetPtr->setStreamIndex(outIndex);
                outFormatContext->writePacket(packetPtr);
            } else {
                packetPtr->rescaleTs(inTimebase, transcodeCtx->decContextInfoPtr->timebase());
                auto framePtrs = transcodeCtx->decContextInfoPtr->decodeFrame(packetPtr);
                for (const auto &framePtr : std::as_const(framePtrs)) {
                    if (!transcodeCtx->filterPtr->isInitialized()) {
                        initFilters(stream_index, framePtr);
                    }
                    filterEncodeWriteframe(framePtr, stream_index);
                }

                calculatePts(packetPtr, decContextInfoPtr.data());
                addPropertyChangeEvent(new PositionEvent(packetPtr->pts()));
                if (transcodeCtx->decContextInfoPtr->mediaType() == AVMEDIA_TYPE_VIDEO) {
                    fpsPtr->update();
                }
            }
        }
    }

    void addPropertyChangeEvent(PropertyChangeEvent *event)
    {
        propertyChangeEventQueue.push_back(PropertyChangeEventPtr(event));
        while (propertyChangeEventQueue.size() > maxPropertyEventQueueSize.load()) {
            propertyChangeEventQueue.take();
        }
        emit q_ptr->eventIncrease();
    }

    void reset()
    {
        if (!transcodeContexts.isEmpty()) {
            qDeleteAll(transcodeContexts);
            transcodeContexts.clear();
        }
        inFormatContext->close();
        outFormatContext->close();

        fpsPtr->reset();
    }

    static auto getKeyFrame(FormatContext *formatContext,
                            AVContextInfo *videoInfo,
                            qint64 timestamp,
                            FramePtr &outPtr) -> bool
    {
        PacketPtr packetPtr(new Packet);
        if (!formatContext->readFrame(packetPtr)) {
            return false;
        }
        if (!formatContext->checkPktPlayRange(packetPtr)) {
        } else if (packetPtr->streamIndex() == videoInfo->index()
                   && ((videoInfo->stream()->disposition & AV_DISPOSITION_ATTACHED_PIC) == 0)
                   && packetPtr->isKey()) {
            auto framePtrs = videoInfo->decodeFrame(packetPtr);
            for (const auto &framePtr : std::as_const(framePtrs)) {
                if (nullptr == framePtr || !framePtr->isKey()) {
                    continue;
                }
                calculatePts(framePtr, videoInfo, formatContext);
                auto pts = framePtr->pts();
                if (timestamp > pts) {
                    continue;
                }
                outPtr = framePtr;
            }
        }
        return true;
    }

    Transcoder *q_ptr;

    QString inFilePath;
    QString outFilepath;
    FormatContext *inFormatContext;
    FormatContext *outFormatContext;
    QList<TranscoderContext *> transcodeContexts{};
    QString subtitleFilename;

    QPair<qint64, qint64> range;

    EncodeContexts decodeContexts;
    EncodeContexts encodeContexts;

    std::atomic_bool runing = true;
    QScopedPointer<Utils::Fps> fpsPtr;

    bool gpuDecode = true;

    Utils::ConcurrentQueue<PropertyChangeEventPtr> propertyChangeEventQueue;
    std::atomic<size_t> maxPropertyEventQueueSize = 100;

    FramePtrList previewFrames;
    QThreadPool *threadPool;
};

Transcoder::Transcoder(QObject *parent)
    : QThread{parent}
    , d_ptr(new TranscoderPrivate(this))
{
    av_log_set_level(AV_LOG_INFO);
}

Transcoder::~Transcoder()
{
    d_ptr->threadPool->clear();
    stopTranscode();
}

void Transcoder::setInFilePath(const QString &filePath)
{
    d_ptr->inFilePath = filePath;
}

void Transcoder::parseInputFile()
{
    d_ptr->reset();
    d_ptr->threadPool->start([this] { d_ptr->openInputFile(true); });
}

auto Transcoder::duration() const -> qint64
{
    return d_ptr->inFormatContext->duration();
}

auto Transcoder::mediaInfo() -> MediaInfo
{
    if (d_ptr->inFilePath.isEmpty()) {
        d_ptr->addPropertyChangeEvent(new ErrorEvent(tr("Input file is empty!")));
        return {};
    }
    return d_ptr->inFormatContext->mediaInfo();
}

void Transcoder::startPreviewFrames(int count)
{
    d_ptr->threadPool->start(new PreviewCountTask(d_ptr->inFilePath, count, this));
}

void Transcoder::setPreviewFrames(const FramePtrList &framePtrs)
{
    QMetaObject::invokeMethod(
        this,
        [this, framePtrs]() {
            d_ptr->previewFrames = framePtrs;
            auto *propertChangeEvent = new PropertyChangeEvent;
            propertChangeEvent->setType(PropertyChangeEvent::EventType::PreviewFramesChanged);
            d_ptr->addPropertyChangeEvent(propertChangeEvent);
        },
        Qt::QueuedConnection);
}

auto Transcoder::previewFrames() const -> FramePtrList
{
    return d_ptr->previewFrames;
}

void Transcoder::setOutFilePath(const QString &filepath)
{
    d_ptr->outFilepath = filepath;
}

void Transcoder::setGpuDecode(bool enable)
{
    d_ptr->gpuDecode = enable;
}

auto Transcoder::isGpuDecode() -> bool
{
    return d_ptr->gpuDecode;
}

void Transcoder::setEncodeContexts(const EncodeContexts &encodeContexts)
{
    for (int i = 0; i < encodeContexts.size(); i++) {
        auto index = encodeContexts.at(i).streamIndex;
        Q_ASSERT(i == index || index < 0);
    }
    d_ptr->encodeContexts = encodeContexts;
}

auto Transcoder::decodeContexts() const -> EncodeContexts
{
    return d_ptr->decodeContexts;
}

void Transcoder::setRange(const QPair<qint64, qint64> &range)
{
    d_ptr->range = range;
}

void Transcoder::setSubtitleFilename(const QString &filename)
{
    Q_ASSERT(QFile::exists(filename));
    d_ptr->subtitleFilename = filename;
    // only for windwos
    d_ptr->subtitleFilename.replace('/', "\\\\");
    auto index = d_ptr->subtitleFilename.indexOf(":\\");
    if (index > 0) {
        d_ptr->subtitleFilename.insert(index, ('\\'));
    }
}

void Transcoder::startTranscode()
{
    stopTranscode();
    d_ptr->runing = true;
    start();
}

void Transcoder::stopTranscode()
{
    d_ptr->runing = false;
    if (isRunning()) {
        quit();
        wait();
    }
    d_ptr->reset();
}

auto Transcoder::fps() -> float
{
    return d_ptr->fpsPtr->getFps();
}

void Transcoder::setPropertyEventQueueMaxSize(size_t size)
{
    d_ptr->maxPropertyEventQueueSize.store(size);
}

auto Transcoder::propertEventyQueueMaxSize() const -> size_t
{
    return d_ptr->maxPropertyEventQueueSize.load();
}

auto Transcoder::propertyChangeEventSize() const -> size_t
{
    return d_ptr->propertyChangeEventQueue.size();
}

auto Transcoder::takePropertyChangeEvent() -> PropertyChangeEventPtr
{
    return d_ptr->propertyChangeEventQueue.take();
}

void Transcoder::run()
{
    QElapsedTimer timer;
    timer.start();
    qInfo() << "Start Transcoding";
    d_ptr->reset();
    if (!d_ptr->openInputFile(false)) {
        d_ptr->addPropertyChangeEvent(new ErrorEvent(tr("Open input file failed!")));
        return;
    }
    if (!d_ptr->openOutputFile()) {
        d_ptr->addPropertyChangeEvent(new ErrorEvent(tr("Open ouput file failed!")));
        return;
    }
    d_ptr->initAudioFifo();
    d_ptr->loop();
    d_ptr->cleanup();

    auto text = tr("Finish Transcoding: %1.")
                    .arg(QTime::fromMSecsSinceStartOfDay(timer.elapsed()).toString("hh:mm:ss.zzz"));
    qInfo() << text;
    if (d_ptr->runing.load()) {
        d_ptr->openInputFile(false);
    }
}

} // namespace Ffmpeg
