#ifndef SUBTITLEDISPLAY_H
#define SUBTITLEDISPLAY_H

#include "decoder.h"
#include "subtitle.h"

namespace Ffmpeg {

class VideoRender;
class Ass;

class SubtitleDisplay : public Decoder<SubtitlePtr>
{
public:
    explicit SubtitleDisplay(QObject *parent = nullptr);
    ~SubtitleDisplay() override;

    void setVideoResolutionRatio(const QSize &size);

    void setVideoRenders(const QList<VideoRender *> &videoRenders);

protected:
    void runDecoder() override;

private:
    class SubtitleDisplayPrivate;
    QScopedPointer<SubtitleDisplayPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // SUBTITLEDISPLAY_H
