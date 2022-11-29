#ifndef DECODERAUDIOFRAME_H
#define DECODERAUDIOFRAME_H

#include "decoder.h"
#include "frame.hpp"

#include <QAudio>
#include <QAudioFormat>

class QAudioDevice;

namespace Ffmpeg {

class DecoderAudioFrame : public Decoder<Frame *>
{
    Q_OBJECT
public:
    explicit DecoderAudioFrame(QObject *parent = nullptr);
    ~DecoderAudioFrame();

    void stopDecoder() override;

    void pause(bool state) override;
    bool isPause();

    void setVolume(qreal volume);

    void setSpeed(double speed) override;

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
    void checkSeek(QElapsedTimer &timer, qint64 &pauseTime);
    void checkSpeed(QElapsedTimer &timer, qint64 &pauseTime);

    void writeToDevice(QByteArray &audioBuf);
    QAudioFormat resetAudioOutput();

    struct DecoderAudioFramePrivate;
    QScopedPointer<DecoderAudioFramePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // DECODERAUDIOFRAME_H
