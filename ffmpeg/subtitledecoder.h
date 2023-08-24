#ifndef SUBTITLEDECODER_H
#define SUBTITLEDECODER_H

#include "decoder.h"
#include "packet.h"

namespace Ffmpeg {

class VideoRender;

class SubtitleDecoder : public Decoder<PacketPtr>
{
public:
    explicit SubtitleDecoder(QObject *parent = nullptr);
    ~SubtitleDecoder();

    void seek(qint64 seekTime) override;

    void pause(bool state) override;

    void setVideoResolutionRatio(const QSize &size);

    void setVideoRenders(QVector<VideoRender *> videoRenders);

protected:
    void runDecoder() override;

private:
    class SubtitleDecoderPrivate;
    QScopedPointer<SubtitleDecoderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // SUBTITLEDECODER_H
