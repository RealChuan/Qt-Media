#pragma once

#include "decoder.h"
#include "packet.hpp"

namespace Ffmpeg {

class VideoRender;

class VideoDecoder : public Decoder<PacketPtr>
{
    Q_OBJECT
public:
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder() override;

    void setVideoRenders(const QList<VideoRender *> &videoRenders);

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
