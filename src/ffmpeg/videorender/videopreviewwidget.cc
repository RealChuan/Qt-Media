#include "videopreviewwidget.hpp"
#include "ffmpegutils.hpp"

#include <ffmpeg/averrormanager.hpp>
#include <ffmpeg/codeccontext.h>
#include <ffmpeg/decoder.h>
#include <ffmpeg/previewtask.hpp>
#include <ffmpeg/videodecoder.h>
#include <ffmpeg/videoframeconverter.hpp>

#include <QPainter>
#include <QRunnable>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

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
    FramePtr framePtr;
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
        new PreviewOneTask(filepath, videoIndex, timestamp, d_ptr->taskId.loadRelaxed(), this));
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

void VideoPreviewWidget::setDisplayImage(const FramePtr &framePtr,
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
        [this, img, pts, framePtr, chapterText] {
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
        [this, text] {
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
