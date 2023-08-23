#include "videopreviewwidget.hpp"

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
    PreviewTask(const QString &filepath,
                int videoIndex,
                qint64 timestamp,
                int taskId,
                VideoPreviewWidget *videoPreviewWidget)
        : QRunnable()
        , m_filepath(filepath)
        , m_videoIndex(videoIndex)
        , m_timestamp(timestamp)
        , m_taskId(taskId)
        , m_videoPreviewWidgetPtr(videoPreviewWidget)
    {
        setAutoDelete(true);
    }
    ~PreviewTask() { m_runing = false; }

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
        formatCtxPtr->seek(m_timestamp / AV_TIME_BASE);
        videoInfoPtr->codecCtx()->flush();
        formatCtxPtr->discardStreamExcluded({-1, m_videoIndex, -1});

        loop(formatCtxPtr.data(), videoInfoPtr.data());
    }

private:
    void loop(FormatContext *formatContext, AVContextInfo *videoInfo)
    {
        while (!m_videoPreviewWidgetPtr.isNull()
               && m_taskId == m_videoPreviewWidgetPtr->currentTaskId()) {
            auto framePtr(getKeyFrame(formatContext, videoInfo));
            if (!m_runing) {
                return;
            }
            if (framePtr.isNull()) {
                continue;
            }
            auto dstSize = QSize(framePtr->avFrame()->width, framePtr->avFrame()->height);
            if (m_videoPreviewWidgetPtr.isNull()) {
                return;
            } else {
                dstSize.scale(m_videoPreviewWidgetPtr->size()
                                  * m_videoPreviewWidgetPtr->devicePixelRatio(),
                              Qt::KeepAspectRatio);
            }
            auto dst_pix_fmt = AV_PIX_FMT_RGB32;
            QScopedPointer<VideoFrameConverter> frameConverterPtr(
                new VideoFrameConverter(videoInfo->codecCtx(), dstSize, dst_pix_fmt));
            QSharedPointer<Frame> frameRgbPtr(new Frame);
            frameRgbPtr->imageAlloc(dstSize, dst_pix_fmt);
            //frameConverterPtr->flush(framePtr.data(), dstSize);
            frameConverterPtr->scale(framePtr.data(), frameRgbPtr.data());
            auto image = frameRgbPtr->toImage();
            if (!m_videoPreviewWidgetPtr.isNull()
                && m_taskId == m_videoPreviewWidgetPtr->currentTaskId()) {
                image.setDevicePixelRatio(m_videoPreviewWidgetPtr->devicePixelRatio());
                m_videoPreviewWidgetPtr->setDisplayImage(frameRgbPtr, image, framePtr->pts());
            }
            return;
        }
    }

    QSharedPointer<Frame> getKeyFrame(FormatContext *formatContext, AVContextInfo *videoInfo)
    {
        QSharedPointer<Frame> framePtr;
        QScopedPointer<Packet> packetPtr(new Packet);
        if (!formatContext->readFrame(packetPtr.get()) || m_videoPreviewWidgetPtr.isNull()) {
            m_runing = false;
            return framePtr;
        }
        if (!formatContext->checkPktPlayRange(packetPtr.get())) {
        } else if (packetPtr->streamIndex() == videoInfo->index()
                   && !(videoInfo->stream()->disposition & AV_DISPOSITION_ATTACHED_PIC)
                   && packetPtr->isKey()) {
            auto frames(videoInfo->decodeFrame(packetPtr.data()));
            for (auto frame : frames) {
                QSharedPointer<Frame> ptr(frame);
                if (!ptr->isKey() && framePtr.isNull()) {
                    continue;
                }
                Ffmpeg::calculateTime(ptr.data(), videoInfo, formatContext);
                auto pts = ptr->pts();
                if (m_timestamp > pts) {
                    continue;
                }
                framePtr = ptr;
            }
        }
        return framePtr;
    }

    QString m_filepath;
    int m_videoIndex;
    qint64 m_timestamp;
    int m_taskId = 0;
    QPointer<VideoPreviewWidget> m_videoPreviewWidgetPtr;
    volatile bool m_runing = true;
};

class VideoPreviewWidget::VideoPreviewWidgetPrivate
{
public:
    VideoPreviewWidgetPrivate(QWidget *parent)
        : owner(parent)
    {
        threadPool = new QThreadPool(owner);
        threadPool->setMaxThreadCount(2);
    }
    ~VideoPreviewWidgetPrivate()
    {
        qDebug() << "Task ID: " << taskId.loadRelaxed() << "Vaild Count: " << vaildCount;
    }

    QWidget *owner;

    QImage image;
    qint64 timestamp;
    qint64 duration;
    QSharedPointer<Frame> frame;

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
    d_ptr->frame.reset();
    update();
}

void VideoPreviewWidget::clearAllTask()
{
    d_ptr->threadPool->clear();
}

void VideoPreviewWidget::setDisplayImage(QSharedPointer<Frame> frame,
                                         const QImage &image,
                                         qint64 pts)
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
            d_ptr->frame = frame;
            update();
            ++d_ptr->vaildCount;
        },
        Qt::QueuedConnection);
}

int VideoPreviewWidget::currentTaskId() const
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
        return;
    }
    paintImage(&painter);

    paintTime(&painter);
}

void VideoPreviewWidget::paintWaiting(QPainter *painter)
{
    QFont font;
    font.setPixelSize(height() / 5);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->drawText(rect(), tr("Waiting..."), QTextOption(Qt::AlignCenter));
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

void VideoPreviewWidget::paintTime(QPainter *painter)
{
    QFont font;
    font.setPixelSize(height() / 9);
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->setOpacity(0.8);
    auto timeStr = QString("%1/%2").arg(
        QTime::fromMSecsSinceStartOfDay(d_ptr->timestamp / 1000).toString("hh:mm:ss"),
        QTime::fromMSecsSinceStartOfDay(d_ptr->duration / 1000).toString("hh:mm:ss"));
    painter->drawText(rect().adjusted(0, 5, 0, 0), timeStr, Qt::AlignTop | Qt::AlignHCenter);
}

} // namespace Ffmpeg
