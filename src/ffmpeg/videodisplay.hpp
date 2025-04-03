#ifndef VIDEODISPLAY_H
#define VIDEODISPLAY_H

#include "decoder.h"
#include "frame.hpp"

namespace Ffmpeg {

class VideoRender;

class VideoDisplay : public Decoder<FramePtr>
{
    Q_OBJECT
public:
    explicit VideoDisplay(QObject *parent = nullptr);
    ~VideoDisplay() override;

    void setVideoRenders(const QList<VideoRender *> &videoRenders);

    void setMasterClock();

signals:
    void positionChanged(qint64 position); // microsecond

protected:
    void runDecoder() override;

private:
    class VideoDisplayPrivate;
    QScopedPointer<VideoDisplayPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // VIDEODISPLAY_H
