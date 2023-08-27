#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include "decoder.h"
#include "packet.h"

namespace Ffmpeg {

class VideoRender;

class VideoDecoder : public Decoder<PacketPtr>
{
    Q_OBJECT
public:
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder();

    void setVideoRenders(QVector<VideoRender *> videoRenders);

    void setMasterClock();

signals:
    void positionChanged(qint64 position); // microsecond

protected:
    void runDecoder() override;

private:
    class VideoDecoderPrivate;
    QScopedPointer<VideoDecoderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // VIDEODECODER_H
