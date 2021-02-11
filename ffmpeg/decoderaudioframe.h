#ifndef DECODERAUDIOFRAME_H
#define DECODERAUDIOFRAME_H

#include "decoder.h"
#include "playframe.h"

namespace Ffmpeg {

class DecoderAudioFramePrivate;
class DecoderAudioFrame : public Decoder<PlayFrame>
{
    Q_OBJECT
public:
    DecoderAudioFrame(QObject *parent = nullptr);
    ~DecoderAudioFrame();

    void stopDecoder() override;

    void pause(bool state);
    bool isPause();

    static double audioClock();

signals:
    void positionChanged(qint64 position); // ms

protected:
    void runDecoder() override;

private:
    void checkPause(qint64 &pauseTime);
    void checkSeek(QElapsedTimer &timer, qint64 &pauseTime);
    void writeToDevice(QByteArray &audioBuf);

    QScopedPointer<DecoderAudioFramePrivate> d_ptr;
};

}

#endif // DECODERAUDIOFRAME_H
