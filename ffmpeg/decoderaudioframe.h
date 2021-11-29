#ifndef DECODERAUDIOFRAME_H
#define DECODERAUDIOFRAME_H

#include "decoder.h"
#include "playframe.h"

#include <QAudio>
#include <QAudioFormat>

class QAudioDevice;

namespace Ffmpeg {

class DecoderAudioFrame : public Decoder<PlayFrame *>
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

    static double audioClock();

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
    void setAudioClock(double time);

    static QMutex m_mutex;
    static double m_audioClock;

    struct DecoderAudioFramePrivate;
    QScopedPointer<DecoderAudioFramePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // DECODERAUDIOFRAME_H
