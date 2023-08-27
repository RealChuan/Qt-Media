#ifndef DECODERSUBTITLEFRAME_HPP
#define DECODERSUBTITLEFRAME_HPP

#include "decoder.h"
#include "subtitle.h"

namespace Ffmpeg {

class VideoRender;
class Ass;

class DecoderSubtitleFrame : public Decoder<SubtitlePtr>
{
public:
    explicit DecoderSubtitleFrame(QObject *parent = nullptr);
    ~DecoderSubtitleFrame();

    void setVideoResolutionRatio(const QSize &size);

    void setVideoRenders(QVector<VideoRender *> videoRenders);

protected:
    void runDecoder() override;

private:
    void renderFrame(const QSharedPointer<Subtitle> &subtitlePtr);

    class DecoderSubtitleFramePrivate;
    QScopedPointer<DecoderSubtitleFramePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // DECODERSUBTITLEFRAME_HPP
