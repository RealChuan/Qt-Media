#ifndef DECODERVIDEOFRAME_H
#define DECODERVIDEOFRAME_H

#include "decoder.h"
#include "playframe.h"

struct AVFrame;

namespace Ffmpeg {

class DecoderVideoFrame : public Decoder<PlayFrame>
{
    Q_OBJECT
public:
    DecoderVideoFrame(QObject *parent = nullptr);

signals:
    void readyRead(const QPixmap &pixmap);

protected:
    void runDecoder() override;
};

}

#endif // DECODERVIDEOFRAME_H
