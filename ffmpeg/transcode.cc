#include "transcode.hpp"
#include "audiofifo.hpp"
#include "audioframeconverter.h"
#include "avcontextinfo.h"
#include "averrormanager.hpp"
#include "codeccontext.h"
#include "ffmpegutils.hpp"
#include "formatcontext.h"
#include "frame.hpp"
#include "packet.h"

#include <filter/filter.hpp>
#include <filter/filtercontext.hpp>
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

    FilterPtr filterPtr;

    QSharedPointer<AudioFifo> audioFifoPtr;
    int64_t audioPts = 0;
};

auto init_filter(TranscodeContext *transcodeContext, const char *filter_spec, Frame *frame) -> bool
{
    auto fliterPtr = transcodeContext->filterPtr;
    if (fliterPtr->isInitialized()) {
        return true;
    }

    auto *dec_ctx = transcodeContext->decContextInfoPtr->codecCtx()->avCodecCtx();
    auto *enc_ctx = transcodeContext->encContextInfoPtr->codecCtx()->avCodecCtx();
    switch (transcodeContext->decContextInfoPtr->mediaType()) {
    case AVMEDIA_TYPE_VIDEO: {
        fliterPtr->init(AVMEDIA_TYPE_VIDEO, frame);
        auto pix_fmt = transcodeContext->encContextInfoPtr->pixfmt();
        av_opt_set_bin(fliterPtr->buffersinkCtx()->avFilterContext(),
                       "pix_fmts",
                       reinterpret_cast<uint8_t *>(&pix_fmt),
                       sizeof(pix_fmt),
                       AV_OPT_SEARCH_CHILDREN);
    } break;
    case AVMEDIA_TYPE_AUDIO: {
        fliterPtr->init(AVMEDIA_TYPE_AUDIO, frame);
        auto *avFilterContext = fliterPtr->buffersinkCtx()->avFilterContext();
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

    fliterPtr->config(filter_spec);
    return true;
}

class Transcode::TranscodePrivate
{
public:
    explicit TranscodePrivate(QObject *parent)
        : owner(parent)
        , inFormatContext(new FormatContext(owner))
        , outFormatContext(new FormatContext(owner))
        , fpsPtr(new Utils::Fps)
    {}

    ~TranscodePrivate() { reset(); }

    auto openInputFile() -> bool
    {
        Q_ASSERT(!inFilePath.isEmpty());
        auto ret = inFormatContext->openFilePath(inFilePath);
        if (!ret) {
            return ret;
        }
        inFormatContext->findStream();
        auto stream_num = inFormatContext->streams();
        for (int i = 0; i < stream_num; i++) {
            auto *transContext = new TranscodeContext;
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
                contextInfoPtr->openCodec(useGpuDecode ? AVContextInfo::GpuDecode
                                                       : AVContextInfo::NotUseGpu);
            }
            transContext->decContextInfoPtr = contextInfoPtr;
        }
        inFormatContext->dumpFormat();
        return true;
    }

    auto openOutputFile() -> bool
    {
        Q_ASSERT(!outFilepath.isEmpty());
        auto ret = outFormatContext->openFilePath(outFilepath, FormatContext::WriteOnly);
        if (!ret) {
            return ret;
        }
        outFormatContext->copyChapterFrom(inFormatContext);
        auto stream_num = inFormatContext->streams();
        for (int i = 0; i < stream_num; i++) {
            auto *inStream = inFormatContext->stream(i);
            auto *stream = outFormatContext->createStream();
            if (stream == nullptr) {
                return false;
            }
            av_dict_copy(&stream->metadata, inStream->metadata, 0);
            stream->disposition = inStream->disposition;
            stream->discard = inStream->discard;
            stream->sample_aspect_ratio = inStream->sample_aspect_ratio;
            stream->avg_frame_rate = inStream->avg_frame_rate;
            stream->event_flags = inStream->event_flags;
            auto *transContext = transcodeContexts.at(i);
            auto decContextInfo = transContext->decContextInfoPtr;
            if ((inStream->disposition & AV_DISPOSITION_ATTACHED_PIC) != 0) {
                auto ret = avcodec_parameters_copy(stream->codecpar, inStream->codecpar);
                if (ret < 0) {
                    qErrnoWarning("Copying parameters for stream #%u failed", i);
                    return ret != 0;
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
                auto *codecCtx = contextInfoPtr->codecCtx();
                auto *avCodecCtx = codecCtx->avCodecCtx();
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
                if ((outFormatContext->avFormatContext()->oformat->flags & AVFMT_GLOBALHEADER)
                    != 0) {
                    avCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
                }
                contextInfoPtr->openCodec(AVContextInfo::GpuEncode);
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
        outFormatContext->avioOpen();
        return outFormatContext->writeHeader();
    }

    void initFilters(int stream_index, Frame *frame)
    {
        auto *transcodeCtx = transcodeContexts.at(stream_index);
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

    void initAudioFifo() const
    {
        auto stream_num = inFormatContext->streams();
        for (int i = 0; i < stream_num; i++) {
            auto *transCtx = transcodeContexts.at(i);
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
            if (!transcodeContexts.at(i)->filterPtr->isInitialized()) {
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

    auto filterEncodeWriteframe(Frame *frame, uint stream_index) const -> bool
    {
        auto *transcodeCtx = transcodeContexts.at(stream_index);
        auto framePtrs = transcodeCtx->filterPtr->filterFrame(frame);
        for (const auto &framePtr : framePtrs) {
            framePtr->setPictType(AV_PICTURE_TYPE_NONE);
            if (transcodeCtx->audioFifoPtr.isNull()) {
                encodeWriteFrame(stream_index, 0, framePtr);
            } else {
                fliterAudioFifo(stream_index, framePtr);
            }
        }
        return true;
    }

    void fliterAudioFifo(uint stream_index,
                         const QSharedPointer<Frame> &framePtr,
                         bool finish = false) const
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

    auto addSamplesToFifo(Frame *frame, uint stream_index) const -> bool
    {
        auto *transcodeCtx = transcodeContexts.at(stream_index);
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
        return audioFifoPtr->write(reinterpret_cast<void **>(frame->avFrame()->data),
                                   frame->avFrame()->nb_samples);
    }

    auto takeSamplesFromFifo(uint stream_index, bool finished = false) const
        -> QSharedPointer<Frame>
    {
        auto *transcodeCtx = transcodeContexts.at(stream_index);
        auto audioFifoPtr = transcodeCtx->audioFifoPtr;
        if (audioFifoPtr.isNull()) {
            return nullptr;
        }
        auto *enc_ctx = transcodeCtx->encContextInfoPtr->codecCtx()->avCodecCtx();
        if (audioFifoPtr->size() < enc_ctx->frame_size && !finished) {
            return nullptr;
        }
        const int frame_size = FFMIN(audioFifoPtr->size(), enc_ctx->frame_size);
        QSharedPointer<Frame> framePtr(new Frame);
        auto *frame = framePtr->avFrame();
        frame->nb_samples = frame_size;
        av_channel_layout_copy(&frame->ch_layout, &enc_ctx->ch_layout);
        frame->format = enc_ctx->sample_fmt;
        frame->sample_rate = enc_ctx->sample_rate;
        if (!framePtr->getBuffer()) {
            return nullptr;
        }
        if (!audioFifoPtr->read(reinterpret_cast<void **>(framePtr->avFrame()->data), frame_size)) {
            return nullptr;
        }
        // fix me?
        frame->pts = transcodeCtx->audioPts / av_q2d(transcodeCtx->decContextInfoPtr->timebase())
                     / transcodeCtx->decContextInfoPtr->codecCtx()->avCodecCtx()->sample_rate;
        transcodeCtx->audioPts += frame->nb_samples;
        //qDebug() << "new: " << stream_index << frame->pts;

        return framePtr;
    }

    auto encodeWriteFrame(uint stream_index, int flush, const QSharedPointer<Frame> &framePtr) const
        -> bool
    {
        auto *transcodeCtx = transcodeContexts.at(stream_index);
        std::vector<PacketPtr> packetPtrs{};
        if (flush != 0) {
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

    auto flushEncoder(uint stream_index) const -> bool
    {
        auto *codecCtx
            = transcodeContexts.at(stream_index)->encContextInfoPtr->codecCtx()->avCodecCtx();
        if ((codecCtx->codec->capabilities & AV_CODEC_CAP_DELAY) == 0) {
            return true;
        }
        return encodeWriteFrame(stream_index, 1, nullptr);
    }

    auto setInMediaIndex(AVContextInfo *contextInfo, int index) const -> bool
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
        d_ptr->subtitleFilename.insert(index, ('\\'));
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

auto Transcode::preset() const -> QString
{
    return d_ptr->preset;
}

auto Transcode::presets() const -> QStringList
{
    return d_ptr->presets;
}

void Transcode::setTune(const QString &tune)
{
    Q_ASSERT(d_ptr->tunes.contains(tune));
    d_ptr->tune = tune;
}

auto Transcode::tune() const -> QString
{
    return d_ptr->tune;
}

auto Transcode::tunes() const -> QStringList
{
    return d_ptr->tunes;
}

void Transcode::setProfile(const QString &profile)
{
    Q_ASSERT(d_ptr->profiles.contains(profile));
    d_ptr->profile = profile;
}

auto Transcode::profile() const -> QString
{
    return d_ptr->profile;
}

auto Transcode::profiles() const -> QStringList
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

auto Transcode::fps() -> float
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

void Transcode::buildConnect() const
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
        auto *transcodeCtx = d_ptr->transcodeContexts.at(stream_index);
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
                if (!transcodeCtx->filterPtr->isInitialized()) {
                    d_ptr->initFilters(stream_index, framePtr.data());
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
