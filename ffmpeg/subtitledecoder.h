#ifndef SUBTITLEDECODER_H
#define SUBTITLEDECODER_H

#include "decoder.h"

namespace Ffmpeg {

class SubtitleDecoder : public Decoder
{
    Q_OBJECT
public:
    SubtitleDecoder(QObject *parent = nullptr);
};

}

#endif // SUBTITLEDECODER_H
