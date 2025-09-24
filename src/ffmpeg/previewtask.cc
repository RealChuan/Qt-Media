#include "previewtask.hpp"
#include "avcontextinfo.h"
#include "averrormanager.hpp"
#include "codeccontext.h"
#include "formatcontext.h"
#include "frame.hpp"
#include "packet.h"
#include "transcoder.hpp"
#include "videoframeconverter.hpp"

#include <videorender/videopreviewwidget.hpp>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

static auto getKeyFrame(FormatContext *formatContext,
                        AVContextInfo *videoInfo,
                        qint64 timestamp,
                        FramePtr &outPtr) -> bool
{
    PacketPtr packetPtr(new Packet);
    if (!formatContext->readFrame(packetPtr.get())) {
        return false;
    }
    if (!formatContext->checkPktPlayRange(packetPtr.get())) {
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

class PreviewOneTask::PreviewOneTaskPrivate
{
public:
    explicit PreviewOneTaskPrivate(PreviewOneTask *q)
        : q_ptr(q)
    {}

    void loop(FormatContext *formatContext, AVContextInfo *videoInfo) const
    {
        while (!videoPreviewWidgetPtr.isNull() && taskId == videoPreviewWidgetPtr->currentTaskId()) {
            FramePtr framePtr;
            while (nullptr == framePtr) {
                if (!getKeyFrame(formatContext, videoInfo, timestamp, framePtr)) {
                    qWarning() << "can't get key frame";
                    if (!videoPreviewWidgetPtr.isNull()) {
                        videoPreviewWidgetPtr->setDisplayText(
                            AVErrorManager::instance()->lastErrorString());
                    }
                    return;
                }
            }
            auto dstSize = QSize(framePtr->avFrame()->width, framePtr->avFrame()->height);
            if (videoPreviewWidgetPtr.isNull()) {
                return;
            }
            dstSize.scale(videoPreviewWidgetPtr->size() * videoPreviewWidgetPtr->devicePixelRatio(),
                          Qt::KeepAspectRatio);

            auto dst_pix_fmt = AV_PIX_FMT_RGB32;
            QScopedPointer<VideoFrameConverter> frameConverterPtr(
                new VideoFrameConverter(framePtr, dstSize, dst_pix_fmt));
            FramePtr frameRgbPtr(new Frame);
            frameRgbPtr->imageAlloc(dstSize, dst_pix_fmt);
            //frameConverterPtr->flush(framePtr.data(), dstSize);
            frameConverterPtr->scale(framePtr, frameRgbPtr);
            auto image = frameRgbPtr->toImage();
            auto chapterText = getChapterText(formatContext);
            if (!videoPreviewWidgetPtr.isNull()
                && taskId == videoPreviewWidgetPtr->currentTaskId()) {
                image.setDevicePixelRatio(videoPreviewWidgetPtr->devicePixelRatio());
                videoPreviewWidgetPtr->setDisplayImage(frameRgbPtr,
                                                       image,
                                                       framePtr->pts(),
                                                       chapterText);
            }
            return;
        }
    }

    auto getChapterText(FormatContext *formatContext) const -> QString
    {
        auto chapters = formatContext->mediaInfo().chapters;
        auto timeStamp = timestamp / AV_TIME_BASE;
        for (const auto &chapter : std::as_const(chapters)) {
            if (chapter.startTime <= timeStamp && chapter.endTime >= timeStamp) {
                return chapter.metadatas.value("title");
            }
        }
        return {};
    }

    PreviewOneTask *q_ptr;

    QString filepath;
    int videoIndex;
    qint64 timestamp;
    int taskId = 0;
    QPointer<VideoPreviewWidget> videoPreviewWidgetPtr;
    std::atomic_bool runing = true;
};

PreviewOneTask::PreviewOneTask(const QString &filepath,
                               int videoIndex,
                               qint64 timestamp,
                               int taskId,
                               VideoPreviewWidget *videoPreviewWidget)
    : d_ptr(new PreviewOneTaskPrivate(this))
{
    d_ptr->filepath = filepath;
    d_ptr->videoIndex = videoIndex;
    d_ptr->timestamp = timestamp;
    d_ptr->taskId = taskId;
    d_ptr->videoPreviewWidgetPtr = videoPreviewWidget;

    setAutoDelete(true);
}

PreviewOneTask::~PreviewOneTask()
{
    d_ptr->runing.store(false);
}

void PreviewOneTask::run()
{
    QScopedPointer<FormatContext> formatCtxPtr(new FormatContext);
    if (!formatCtxPtr->openFilePath(d_ptr->filepath) || !formatCtxPtr->findStream()) {
        return;
    }

    QScopedPointer<AVContextInfo> videoInfoPtr(new AVContextInfo);
    videoInfoPtr->setIndex(d_ptr->videoIndex);
    videoInfoPtr->setStream(formatCtxPtr->stream(d_ptr->videoIndex));
    if (!videoInfoPtr->initDecoder(formatCtxPtr->guessFrameRate(d_ptr->videoIndex))) {
        return;
    }
    videoInfoPtr->openCodec(); // 软解
    formatCtxPtr->seek(d_ptr->timestamp);
    videoInfoPtr->codecCtx()->flush();
    formatCtxPtr->discardStreamExcluded({d_ptr->videoIndex});

    d_ptr->loop(formatCtxPtr.data(), videoInfoPtr.data());
}

class PreviewCountTask::PreviewCountTaskPrivate
{
public:
    explicit PreviewCountTaskPrivate(PreviewCountTask *q)
        : q_ptr(q)
    {}

    void loop(FormatContext *formatContext, AVContextInfo *videoInfo) const
    {
        auto step = formatContext->duration() / count;

        FramePtrList framePtrs;
        for (int i = 0; i < count; ++i) {
            if (!runing.load() || transcoderPtr.isNull()) {
                return;
            }

            auto timestamp = i * step;
            if (i == 0) {
                formatContext->seekFirstFrame();
            } else {
                formatContext->seek(timestamp);
            }
            videoInfo->codecCtx()->flush();
            FramePtr framePtr;
            while (nullptr == framePtr) {
                if (!getKeyFrame(formatContext, videoInfo, timestamp, framePtr)) {
                    qWarning() << "can't get key frame";
                    return;
                }
            }
            framePtrs.push_back(framePtr);
        }
        if (!transcoderPtr.isNull()) {
            transcoderPtr->setPreviewFrames(framePtrs);
        }
    }

    PreviewCountTask *q_ptr;

    QString filepath;
    int count;
    QPointer<Transcoder> transcoderPtr;
    std::atomic_bool runing = true;
};

PreviewCountTask::PreviewCountTask(const QString &filepath, int count, Transcoder *transcoder)
    : d_ptr(new PreviewCountTaskPrivate(this))
{
    d_ptr->filepath = filepath;
    d_ptr->count = count;
    d_ptr->transcoderPtr = transcoder;

    setAutoDelete(true);
}

PreviewCountTask::~PreviewCountTask()
{
    d_ptr->runing.store(false);
}

void PreviewCountTask::run()
{
    QScopedPointer<FormatContext> formatCtxPtr(new FormatContext);
    if (!formatCtxPtr->openFilePath(d_ptr->filepath) || !formatCtxPtr->findStream()) {
        return;
    }

    auto videoIndex = formatCtxPtr->findBestStreamIndex(AVMEDIA_TYPE_VIDEO);
    if (videoIndex < 0) {
        qWarning() << "can't find video stream";
        return;
    }
    QScopedPointer<AVContextInfo> videoInfoPtr(new AVContextInfo);
    videoInfoPtr->setIndex(videoIndex);
    videoInfoPtr->setStream(formatCtxPtr->stream(videoIndex));
    if (!videoInfoPtr->initDecoder(formatCtxPtr->guessFrameRate(videoIndex))) {
        return;
    }
    videoInfoPtr->openCodec(AVContextInfo::GpuDecode);
    formatCtxPtr->discardStreamExcluded({videoIndex});
    d_ptr->loop(formatCtxPtr.data(), videoInfoPtr.data());
}

} // namespace Ffmpeg
