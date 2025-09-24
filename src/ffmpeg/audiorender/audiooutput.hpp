#pragma once

#include <ffmpeg/frame.hpp>

#include <QAudio>
#include <QObject>

namespace Ffmpeg {

class AVContextInfo;
class AudioOutput : public QObject
{
    Q_OBJECT
public:
    explicit AudioOutput(AVContextInfo *contextInfo, qreal volume = 0.5, QObject *parent = nullptr);
    ~AudioOutput() override;

public slots:
    void onConvertData(const FramePtr &framePtr);
    void onWrite();
    void onSetVolume(qreal value);

private slots:
    void onStateChanged(QAudio::State state);
    void onAudioOutputsChanged();

private:
    void buildConnect();

    class AudioOutputPrivate;
    QScopedPointer<AudioOutputPrivate> d_ptr;
};

} // namespace Ffmpeg
