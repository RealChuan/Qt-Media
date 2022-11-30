#include "videopreviewwidget.hpp"

#include <ffmpeg/codeccontext.h>
#include <ffmpeg/formatcontext.h>
#include <ffmpeg/frame.hpp>
#include <ffmpeg/frameconverter.hpp>
#include <ffmpeg/hardwaredecode.hpp>
#include <ffmpeg/videodecoder.h>

#include <QPainter>
#include <QRunnable>

namespace Ffmpeg {

class PreviewTask : public QRunnable
{
public:
    PreviewTask(const QString &filepath,
                int videoIndex,
                qint64 timestamp,
                VideoPreviewWidget *videoPreviewWidget)
        : QRunnable()
        , m_filepath(filepath)
        , m_videoIndex(videoIndex)
        , m_timestamp(timestamp)
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
        QScopedPointer<AVContextInfo> videoInfoPtr(new AVContextInfo(AVContextInfo::Video));
        videoInfoPtr->setIndex(m_videoIndex);
        videoInfoPtr->setStream(formatCtxPtr->stream(m_videoIndex));
        if (!videoInfoPtr->findDecoder()) { // 软解
            return;
        }
        formatCtxPtr->seek(m_timestamp);
        videoInfoPtr->flush();

        loop(formatCtxPtr.data(), videoInfoPtr.data());
    }

private:
    void loop(FormatContext *formatContext, AVContextInfo *videoInfo)
    {
        while (m_runing) {
            QScopedPointer<Packet> packetPtr(new Packet);
            if (!formatContext->readFrame(packetPtr.get()) || m_videoPreviewWidgetPtr.isNull()) {
                return;
            }
            if (formatContext->checkPktPlayRange(packetPtr.get()) <= 0) {
            } else if (packetPtr->avPacket()->stream_index == videoInfo->index()
                       && !(videoInfo->stream()->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
                if (!videoInfo->sendPacket(packetPtr.data())) {
                    continue;
                }
                QScopedPointer<Frame> framePtr(new Frame);
                if (!videoInfo->receiveFrame(framePtr.data())) { // 一个packet一个视频帧
                    continue;
                }
                if (videoInfo->isGpuDecode()) {
                    bool ok = false;
                    framePtr.reset(videoInfo->hardWareDecode()->transforFrame(framePtr.get(), ok));
                    //qDebug() << framePtr->avFrame()->format;
                }
                double duration = 0;
                double pts = 0;
                calculateTime(framePtr->avFrame(), duration, pts, videoInfo, formatContext);
                if (m_timestamp > pts) {
                    continue;
                }
                auto dstSize = QSize(framePtr->avFrame()->width, framePtr->avFrame()->height);
                if (m_videoPreviewWidgetPtr.isNull()) {
                    return;
                } else {
                    dstSize.scale(m_videoPreviewWidgetPtr->width(),
                                  m_videoPreviewWidgetPtr->height(),
                                  Qt::KeepAspectRatio);
                }
                QScopedPointer<FrameConverter> frameConverterPtr(
                    new FrameConverter(videoInfo->codecCtx(), dstSize));
                QScopedPointer<Frame> frameRgbPtr(new Frame);
                videoInfo->imageAlloc(*frameRgbPtr.data(), dstSize);
                frameConverterPtr->flush(framePtr.data(), dstSize);
                auto image(frameConverterPtr->scaleToImageRgb32(framePtr.data(),
                                                                frameRgbPtr.data(),
                                                                videoInfo->codecCtx(),
                                                                dstSize));
                if (!m_videoPreviewWidgetPtr.isNull()) {
                    m_videoPreviewWidgetPtr->setDisplayImage(image, pts);
                }
                return;
            }
        }
    }

    void calculateTime(AVFrame *frame,
                       double &duration,
                       double &pts,
                       AVContextInfo *contextInfo,
                       FormatContext *formatContext)
    {
        AVRational tb = contextInfo->stream()->time_base;
        AVRational frame_rate = av_guess_frame_rate(formatContext->avFormatContext(),
                                                    contextInfo->stream(),
                                                    NULL);
        // 当前帧播放时长
        duration = (frame_rate.num && frame_rate.den
                        ? av_q2d(AVRational{frame_rate.den, frame_rate.num})
                        : 0);
        // 当前帧显示时间戳
        pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
        //qDebug() << duration << pts;
    }

    QString m_filepath;
    int m_videoIndex;
    qint64 m_timestamp;
    QPointer<VideoPreviewWidget> m_videoPreviewWidgetPtr;
    volatile bool m_runing = true;
};

struct VideoPreviewWidget::VideoPreviewWidgetPrivate
{
    QImage image;
    qint64 timestamp;
    qint64 duration;
};

VideoPreviewWidget::VideoPreviewWidget(QWidget *parent)
    : QWidget{parent}
    , d_ptr(new VideoPreviewWidgetPrivate)
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
}

VideoPreviewWidget::VideoPreviewWidget(
    const QString &filepath, int videoIndex, qint64 timestamp, qint64 duration, QWidget *parent)
    : QWidget{parent}
    , d_ptr(new VideoPreviewWidgetPrivate)
{
    Q_ASSERT(videoIndex >= 0);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    QThreadPool::globalInstance()->start(new PreviewTask(filepath, videoIndex, timestamp, this));
    d_ptr->timestamp = timestamp;
    d_ptr->duration = duration;
}

VideoPreviewWidget::~VideoPreviewWidget() {}

void VideoPreviewWidget::startPreview(const QString &filepath,
                                      int videoIndex,
                                      qint64 timestamp,
                                      qint64 duration)
{
    Q_ASSERT(videoIndex >= 0);
    QThreadPool::globalInstance()->start(new PreviewTask(filepath, videoIndex, timestamp, this));
    d_ptr->timestamp = timestamp;
    d_ptr->duration = duration;
    d_ptr->image = QImage();
}

void VideoPreviewWidget::setDisplayImage(const QImage &image, qint64 pts)
{
    d_ptr->timestamp = pts;
    d_ptr->image = image.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QMetaObject::invokeMethod(
        this, [this] { update(); }, Qt::QueuedConnection);
}

void VideoPreviewWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    auto rect = this->rect();
    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    painter.drawRect(rect);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform
                           | QPainter::TextAntialiasing);
    if (d_ptr->image.isNull()) {
        QFont font;
        font.setPixelSize(height() / 5);
        painter.setFont(font);
        painter.setPen(Qt::white);
        painter.drawText(rect, tr("Waiting..."), QTextOption(Qt::AlignCenter));
        return;
    }

    int x = (width() - d_ptr->image.width()) / 2;
    int y = (height() - d_ptr->image.height()) / 2;
    painter.drawImage(QRect(x, y, d_ptr->image.width(), d_ptr->image.height()), d_ptr->image);

    QFont font;
    font.setPixelSize(height() / 9);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.setOpacity(0.8);
    auto timeStr = QString("%1/%2").arg(
        QTime::fromMSecsSinceStartOfDay(d_ptr->timestamp * 1000).toString("hh:mm:ss"),
        QTime::fromMSecsSinceStartOfDay(d_ptr->duration * 1000).toString("hh:mm:ss"));
    painter.drawText(rect.adjusted(0, 5, 0, 0), timeStr, Qt::AlignTop | Qt::AlignHCenter);
}

} // namespace Ffmpeg
