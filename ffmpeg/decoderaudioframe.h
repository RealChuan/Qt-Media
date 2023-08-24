#ifndef DECODERAUDIOFRAME_H
#define DECODERAUDIOFRAME_H

#include "decoder.h"
#include "frame.hpp"

#include <QAudio>
#include <QAudioFormat>

class QAudioDevice;

namespace Ffmpeg {

class DecoderAudioFrame : public Decoder<FramePtr>
{
    Q_OBJECT
public:
    explicit DecoderAudioFrame(QObject *parent = nullptr);
    ~DecoderAudioFrame();

    void stopDecoder() override;

    bool seek(qint64 seekTime) override;

    void pause(bool state) override;
    auto isPause() -> bool;

    void setVolume(qreal volume);

    void setIsLocalFile(bool isLocalFile);

signals:
    void positionChanged(qint64 position); // ms

private slots:
    void onStateChanged(QAudio::State state);
    void onAudioOutputsChanged();

protected:
    void runDecoder() override;

private:
    void buildConnect();

    void checkDefaultAudioOutput(QAudioDevice &audioDevice);
    void checkPause(qint64 &pauseTime);
    void checkSpeed(QElapsedTimer &timer, qint64 &pauseTime);

    void writeToDevice(QByteArray &audioBuf);
    auto resetAudioOutput() -> QAudioFormat;

    class DecoderAudioFramePrivate;
    QScopedPointer<DecoderAudioFramePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // DECODERAUDIOFRAME_H
