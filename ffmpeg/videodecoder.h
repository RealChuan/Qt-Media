#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include "decoder.h"

namespace Ffmpeg {

class VideoDecoder : public Decoder
{
    Q_OBJECT
public:
    VideoDecoder(QObject *parent = nullptr);

signals:
    void readyRead(const QPixmap &pixmap);

protected:
    void runDecoder() override;

private:
    void calculateTime(PlayFrame &frame, double &duration, double &pts);
};

}

#endif // VIDEODECODER_H
