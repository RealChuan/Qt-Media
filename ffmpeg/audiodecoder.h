#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include "decoder.h"
#include "packet.h"

namespace Ffmpeg {

class AudioDecoder : public Decoder<PacketPtr>
{
    Q_OBJECT
public:
    explicit AudioDecoder(QObject *parent = nullptr);
    ~AudioDecoder() override;

    void setVolume(qreal volume);

    void setMasterClock();

signals:
    void positionChanged(qint64 position); // ms

protected:
    void runDecoder() override;

private:
    class AudioDecoderPrivate;
    QScopedPointer<AudioDecoderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // AUDIODECODER_H
