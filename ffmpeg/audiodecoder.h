#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include "decoder.h"

namespace Ffmpeg {

class AudioDecoderPrivate;
class AudioDecoder : public Decoder
{
    Q_OBJECT
public:
    explicit AudioDecoder(QObject *parent = nullptr);
    ~AudioDecoder() override;

protected:
    void runDecoder() override;

private:
    void calculateTime(PlayFrame &frame, double &duration, double &pts, int64_t &pos);

    QScopedPointer<AudioDecoderPrivate> d_ptr;
};

}

#endif // AUDIODECODER_H
