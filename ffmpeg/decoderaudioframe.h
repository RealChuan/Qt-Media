#ifndef DECODERAUDIOFRAME_H
#define DECODERAUDIOFRAME_H

#include "decoder.h"
#include "playframe.h"

namespace Ffmpeg {

class DecoderAudioFramePrivate;
class DecoderAudioFrame : public Decoder<PlayFrame>
{
public:
    DecoderAudioFrame(QObject *parent = nullptr);
    ~DecoderAudioFrame();

    static double audioClock();

protected:
    void runDecoder() override;

private:
    void calculateTime(PlayFrame &frame, double &duration, double &pts, int64_t &pos);

    QScopedPointer<DecoderAudioFramePrivate> d_ptr;
};

}

#endif // DECODERAUDIOFRAME_H
