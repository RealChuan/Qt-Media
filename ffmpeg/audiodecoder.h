#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include "decoder.h"
#include "packet.h"

namespace Ffmpeg {

class AudioDecoderPrivate;
class AudioDecoder : public Decoder<Packet *>
{
    Q_OBJECT
public:
    explicit AudioDecoder(QObject *parent = nullptr);
    ~AudioDecoder() override;

    void pause(bool state) override;

    void setVolume(qreal volume);

    void setSeek(qint64 seek);

    void setSpeed(double speed) override;

    double audioClock();

    void setIsLocalFile(bool isLocalFile);

signals:
    void positionChanged(qint64 position); // ms

protected:
    void runDecoder() override;

private:
    QScopedPointer<AudioDecoderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // AUDIODECODER_H
