#include "videopreviewwidget.hpp"
#include "ffmpegutils.hpp"

#include <ffmpeg/averrormanager.hpp>
#include <ffmpeg/codeccontext.h>
#include <ffmpeg/decoder.h>
#include <ffmpeg/frame.hpp>
#include <ffmpeg/videodecoder.h>
#include <ffmpeg/videoframeconverter.hpp>

#include <QPainter>
#include <QRunnable>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

class PreviewTask : public QRunnable
{
public:
    explicit PreviewTask(const QString &filepath,
                         int videoIndex,
                         qint64 timestamp,
                         int taskId,
                         VideoPreviewWidget *videoPreviewWidget)
        : m_filepath(filepath)
        , m_videoIndex(videoIndex)
        , m_timestamp(timestamp)
        , m_taskId(taskId)
        , m_videoPreviewWidgetPtr(videoPreviewWidget)
    {
        setAutoDelete(true);
    }
    ~PreviewTask() override { m_runing = false; }

    void run() override
    {
        QScopedPointer<FormatContext> formatCtxPtr(new FormatContext);
        if (!formatCtxPtr->openFilePath(m_filepath)) {
            return;
        }
        if (!formatCtxPtr->findStream()) {
            return;
        }
        QScopedPointer<AVContextInfo> videoInfoPtr(new AVContextInfo);
        videoInfoPtr->setIndex(m_videoIndex);
        videoInfoPtr->setStream(formatCtxPtr->stream(m_videoIndex));
        if (!videoInfoPtr->initDecoder(formatCtxPtr->guessFrameRate(m_videoIndex))) {
            return;
        }
        videoInfoPtr->openCodec(); // 软解
        formatCtxPtr->seek(m_timestamp);
        videoInfoPtr->codecCtx()->flush();
        formatCtxPtr->discardStreamExcluded({m_videoIndex});

        loop(formatCtxPtr.data(), videoInfoPtr.data());
    }

private:
    void loop(FormatContext *formatContext, AVContextInfo *videoInfo)
    {
        while (!m_videoPreviewWidgetPtr.isNull()
               && m_taskId == m_videoPreviewWidgetPtr->currentTaskId()) {
            auto framePtr(getKeyFrame(formatContext, videoInfo));
            if (!m_runing) {
                if (!m_videoPreviewWidgetPtr.isNull()) {
                    m_videoPreviewWidgetPtr->setDisplayText(
                        AVErrorManager::instance()->lastErrorString());
                }
                return;
            }
            if (framePtr.isNull()) {
                continue;
            }
            auto dstSize = QSize(framePtr->avFrame()->width, framePtr->avFrame()->height);
            if (m_videoPreviewWidgetPtr.isNull()) {
                return;
            }
            dstSize.scale(m_videoPreviewWidgetPtr->size()
                              * m_videoPreviewWidgetPtr->devicePixelRatio(),
                          Qt::KeepAspectRatio);

            auto dst_pix_fmt = AV_PIX_FMT_RGB32;
            QScopedPointer<VideoFrameConverter> frameConverterPtr(
                new VideoFrameConverter(framePtr.data(), dstSize, dst_pix_fmt));
            QSharedPointer<Frame> frameRgbPtr(new Frame);
            frameRgbPtr->imageAlloc(dstSize, dst_pix_fmt);
            //frameConverterPtr->flush(framePtr.data(), dstSize);
            frameConverterPtr->scale(framePtr.data(), frameRgbPtr.data());
            auto image = frameRgbPtr->toImage();
            auto chapterText = getChapterText(formatContext);
            if (!m_videoPreviewWidgetPtr.isNull()
                && m_taskId == m_videoPreviewWidgetPtr->currentTaskId()) {
                image.setDevicePixelRatio(m_videoPreviewWidgetPtr->devicePixelRatio());
                m_videoPreviewWidgetPtr->setDisplayImage(frameRgbPtr,
                                                         image,
                                                         framePtr->pts(),
                                                         chapterText);
            }
            return;
        }
    }

    auto getKeyFrame(FormatContext *formatContext, AVContextInfo *videoInfo) -> FramePtr
    {
        FramePtr outPtr;
        PacketPtr packetPtr(new Packet);
        if (!formatContext->readFrame(packetPtr.get()) || m_videoPreviewWidgetPtr.isNull()) {
            m_runing = false;
            return outPtr;
        }
        if (!formatContext->checkPktPlayRange(packetPtr.get())) {
        } else if (packetPtr->streamIndex() == videoInfo->index()
                   && ((videoInfo->stream()->disposition & AV_DISPOSITION_ATTACHED_PIC) == 0)
                   && packetPtr->isKey()) {
            auto framePtrs = videoInfo->decodeFrame(packetPtr);
            for (const auto &framePtr : framePtrs) {
                if (!framePtr->isKey() && framePtr.isNull()) {
                    continue;
                }
                calculatePts(framePtr.data(), videoInfo, formatContext);
                auto pts = framePtr->pts();
                if (m_timestamp > pts) {
                    continue;
                }
                outPtr = framePtr;
            }
        }
        return outPtr;
    }

    auto getChapterText(FormatContext *formatContext) const -> QString
    {
        auto chapters = formatContext->mediaInfo().chapters;
        auto timeStamp = m_timestamp / AV_TIME_BASE;
        for (const auto &chapter : std::as_const(chapters)) {
            if (chapter.startTime <= timeStamp && chapter.endTime >= timeStamp) {
                return chapter.metadatas.value("title");
            }
        }
        return {};
    }

    QString m_filepath;
    int m_videoIndex;
    qint64 m_timestamp;
    int m_taskId = 0;
    QPointer<VideoPreviewWidget> m_videoPreviewWidgetPtr;
    std::atomic_bool m_runing = true;
};

class VideoPreviewWidget::VideoPreviewWidgetPrivate
{
public:
    explicit VideoPreviewWidgetPrivate(VideoPreviewWidget *q)
        : q_ptr(q)
    {
        threadPool = new QThreadPool(q_ptr);
        threadPool->setMaxThreadCount(2);
    }
    ~VideoPreviewWidgetPrivate()
    {
        qDebug() << "Task ID: " << taskId.loadRelaxed() << "Vaild Count: " << vaildCount;
    }

    QWidget *q_ptr;

    QImage image;
    qint64 timestamp;
    qint64 duration;
    QSharedPointer<Frame> framePtr;
    QString chapterText;
    QString displayText;

    QAtomicInt taskId = 0;
    qint64 vaildCount = 0;
    QThreadPool *threadPool;
};

VideoPreviewWidget::VideoPreviewWidget(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new VideoPreviewWidgetPrivate(this))
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
}

VideoPreviewWidget::~VideoPreviewWidget()
{
    clearAllTask();
}

void VideoPreviewWidget::startPreview(const QString &filepath,
                                      int videoIndex,
                                      qint64 timestamp,
                                      qint64 duration)
{
    Q_ASSERT(videoIndex >= 0);
    d_ptr->taskId.ref();
    clearAllTask();
    d_ptr->threadPool->start(
        new PreviewTask(filepath, videoIndex, timestamp, d_ptr->taskId.loadRelaxed(), this));
    d_ptr->timestamp = timestamp;
    d_ptr->duration = duration;
    d_ptr->image = QImage();
    d_ptr->framePtr.reset();
    d_ptr->displayText = tr("Waiting...");
    update();
}

void VideoPreviewWidget::clearAllTask()
{
    d_ptr->threadPool->clear();
}

void VideoPreviewWidget::setDisplayImage(const QSharedPointer<Frame> &framePtr,
                                         const QImage &image,
                                         qint64 pts,
                                         const QString &chapterText)
{
    auto img = image.scaled(size() * devicePixelRatio(),
                            Qt::KeepAspectRatio,
                            Qt::SmoothTransformation);
    img.setDevicePixelRatio(devicePixelRatio());
    QMetaObject::invokeMethod(
        this,
        [=] {
            d_ptr->timestamp = pts;
            d_ptr->image = img;
            d_ptr->framePtr = framePtr;
            d_ptr->chapterText = chapterText;
            update();
            ++d_ptr->vaildCount;
        },
        Qt::QueuedConnection);
}

void VideoPreviewWidget::setDisplayText(const QString &text)
{
    QMetaObject::invokeMethod(
        this,
        [=] {
            d_ptr->displayText = text;
            update();
        },
        Qt::QueuedConnection);
}

auto VideoPreviewWidget::currentTaskId() const -> int
{
    return d_ptr->taskId.loadRelaxed();
}

void VideoPreviewWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    painter.drawRect(rect());
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform
                           | QPainter::TextAntialiasing);
    if (d_ptr->image.isNull()) {
        paintWaiting(&painter);
        paintText(&painter);
        return;
    }
    paintImage(&painter);

    paintText(&painter);
}

void VideoPreviewWidget::paintWaiting(QPainter *painter)
{
    QFont font;
    font.setPixelSize(height() / 5);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->drawText(rect(), d_ptr->displayText, QTextOption(Qt::AlignCenter));
}

void VideoPreviewWidget::paintImage(QPainter *painter)
{
    auto size = d_ptr->image.size().scaled(this->size(), Qt::KeepAspectRatio);
    auto rect = QRect((width() - size.width()) / 2,
                      (height() - size.height()) / 2,
                      size.width(),
                      size.height());
    painter->drawImage(rect, d_ptr->image);
}

void VideoPreviewWidget::paintText(QPainter *painter)
{
    QFont font;
    font.setPixelSize(height() / 9);
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->setOpacity(0.8);
    auto rect = this->rect().adjusted(0, 5, 0, -5);

    if (!d_ptr->chapterText.isEmpty()) {
        painter->drawText(rect, d_ptr->chapterText, Qt::AlignTop | Qt::AlignHCenter);
    }

    auto timeStr = QString("%1 / %2").arg(
        QTime::fromMSecsSinceStartOfDay(d_ptr->timestamp / 1000).toString("hh:mm:ss"),
        QTime::fromMSecsSinceStartOfDay(d_ptr->duration / 1000).toString("hh:mm:ss"));
    painter->drawText(rect, timeStr, Qt::AlignBottom | Qt::AlignHCenter);
}

} // namespace Ffmpeg
