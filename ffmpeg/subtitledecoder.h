#ifndef SUBTITLEDECODER_H
#define SUBTITLEDECODER_H

#include "decoder.h"
#include "packet.h"

namespace Ffmpeg {

class SubtitleDecoder : public Decoder<Packet *>
{
    Q_OBJECT
public:
    explicit SubtitleDecoder(QObject *parent = nullptr);
    ~SubtitleDecoder();

    void pause(bool state) override;

    void setSpeed(double speed) override;

protected:
    void runDecoder() override;

private:
    class SubtitleDecoderPrivate;
    QScopedPointer<SubtitleDecoderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // SUBTITLEDECODER_H
