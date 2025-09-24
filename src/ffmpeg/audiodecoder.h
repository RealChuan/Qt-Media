#pragma once

#include "decoder.h"
#include "packet.hpp"

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
