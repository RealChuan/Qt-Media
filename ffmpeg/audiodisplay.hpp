#ifndef AUDIODISPLAY_H
#define AUDIODISPLAY_H

#include "decoder.h"
#include "frame.hpp"

namespace Ffmpeg {

class AudioDisplay : public Decoder<FramePtr>
{
    Q_OBJECT
public:
    explicit AudioDisplay(QObject *parent = nullptr);
    ~AudioDisplay();

    void setVolume(qreal volume);

    void setMasterClock();

signals:
    void positionChanged(qint64 position); // microsecond

protected:
    void runDecoder() override;

private:
    class AudioDisplayPrivate;
    QScopedPointer<AudioDisplayPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // AUDIODISPLAY_H
