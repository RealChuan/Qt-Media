#ifndef DECODERAUDIOFRAME_H
#define DECODERAUDIOFRAME_H

#include "decoder.h"
#include "playframe.h"

#include <QAudioFormat>

namespace Ffmpeg {

class DecoderAudioFramePrivate;
class DecoderAudioFrame : public Decoder<PlayFrame>
{
    Q_OBJECT
public:
    DecoderAudioFrame(QObject *parent = nullptr);
    ~DecoderAudioFrame();

    void stopDecoder() override;

    void pause(bool state) override;
    bool isPause();

    void setVolume(qreal volume);

    void setSpeed(double speed) override;

    static double audioClock();

signals:
    void positionChanged(qint64 position); // ms

protected:
    void runDecoder() override;

private:
    void checkPause(qint64 &pauseTime);
    void checkSeek(QElapsedTimer &timer, qint64 &pauseTime);
    void checkSpeed(QElapsedTimer &timer, qint64 &pauseTime);
    void writeToDevice(QByteArray &audioBuf);
    QAudioFormat resetAudioOutput();
    void setAudioClock(double time);

    static QMutex m_mutex;
    static double m_audioClock;
    QScopedPointer<DecoderAudioFramePrivate> d_ptr;
};

}

#endif // DECODERAUDIOFRAME_H
