#ifndef SUBTITLEDECODER_H
#define SUBTITLEDECODER_H

#include "decoder.h"
#include "packet.h"

namespace Ffmpeg {

class SubtitleDecoder : public Decoder<Packet>
{
    Q_OBJECT
public:
    SubtitleDecoder(QObject *parent = nullptr);
};

}

#endif // SUBTITLEDECODER_H
