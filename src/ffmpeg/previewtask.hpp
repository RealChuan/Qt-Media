#pragma once

#include <QRunnable>
#include <QtCore>

namespace Ffmpeg {

class VideoPreviewWidget;
class PreviewOneTask : public QRunnable
{
public:
    explicit PreviewOneTask(const QString &filepath,
                            int videoIndex,
                            qint64 timestamp,
                            int taskId,
                            VideoPreviewWidget *videoPreviewWidget);
    ~PreviewOneTask() override;

    void run() override;

private:
    class PreviewOneTaskPrivate;
    QScopedPointer<PreviewOneTaskPrivate> d_ptr;
};

class Transcoder;
class PreviewCountTask : public QRunnable
{
public:
    explicit PreviewCountTask(const QString &filepath, int count, Transcoder *transcoder);
    ~PreviewCountTask() override;

    void run() override;

private:
    class PreviewCountTaskPrivate;
    QScopedPointer<PreviewCountTaskPrivate> d_ptr;
};

} // namespace Ffmpeg
