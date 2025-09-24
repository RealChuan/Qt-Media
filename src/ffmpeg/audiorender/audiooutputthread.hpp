#pragma once

#include <ffmpeg/frame.hpp>

#include <QThread>

namespace Ffmpeg {

class AVContextInfo;

class AudioOutputThread : public QThread
{
    Q_OBJECT
public:
    explicit AudioOutputThread(QObject *parent = nullptr);
    ~AudioOutputThread() override;

    void openOutput(AVContextInfo *contextInfo, qreal volume);
    void closeOutput();

signals:
    void convertData(const FramePtr &frameptr);
    void wirteData();
    void volumeChanged(qreal value);

protected:
    void run() override;

private:
    class AudioOutputThreadPrivate;
    QScopedPointer<AudioOutputThreadPrivate> d_ptr;
};

} // namespace Ffmpeg
