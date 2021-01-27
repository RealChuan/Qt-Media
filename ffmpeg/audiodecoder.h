#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include "decoder.h"
#include "packet.h"

namespace Ffmpeg {

class AudioDecoderPrivate;
class AudioDecoder : public Decoder<Packet>
{
    Q_OBJECT
public:
    explicit AudioDecoder(QObject *parent = nullptr);
    ~AudioDecoder() override;

signals:
    void positionChanged(qint64 position); // ms

protected:
    void runDecoder() override;

private:
    QScopedPointer<AudioDecoderPrivate> d_ptr;
};

}

#endif // AUDIODECODER_H
