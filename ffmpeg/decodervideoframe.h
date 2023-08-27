#ifndef DECODERVIDEOFRAME_H
#define DECODERVIDEOFRAME_H

#include "decoder.h"
#include "frame.hpp"

namespace Ffmpeg {

class VideoRender;

class DecoderVideoFrame : public Decoder<FramePtr>
{
    Q_OBJECT
public:
    explicit DecoderVideoFrame(QObject *parent = nullptr);
    ~DecoderVideoFrame();

    void setVideoRenders(QVector<VideoRender *> videoRenders);

    void setMasterClock();

signals:
    void positionChanged(qint64 position); // microsecond

protected:
    void runDecoder() override;

private:
    void renderFrame(const QSharedPointer<Frame> &framePtr);

    class DecoderVideoFramePrivate;
    QScopedPointer<DecoderVideoFramePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // DECODERVIDEOFRAME_H
