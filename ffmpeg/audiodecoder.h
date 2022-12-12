#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include "decoder.h"
#include "packet.h"

namespace Ffmpeg {

class AudioDecoder : public Decoder<Packet *>
{
    Q_OBJECT
public:
    explicit AudioDecoder(QObject *parent = nullptr);
    ~AudioDecoder() override;

    void pause(bool state) override;

    void setVolume(qreal volume);

    void setIsLocalFile(bool isLocalFile);

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
