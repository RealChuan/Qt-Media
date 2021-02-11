#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include "decoder.h"
#include "packet.h"

struct AVFrame;

namespace Ffmpeg {

class VideoDecoderPrivate;
class VideoDecoder : public Decoder<Packet>
{
    Q_OBJECT
public:
    VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder();

    void pause(bool state);

signals:
    void readyRead(const QImage &image);

protected:
    void runDecoder() override;

private:
    QScopedPointer<VideoDecoderPrivate> d_ptr;
};

}

#endif // VIDEODECODER_H
