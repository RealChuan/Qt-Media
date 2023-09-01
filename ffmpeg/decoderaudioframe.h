#ifndef DECODERAUDIOFRAME_H
#define DECODERAUDIOFRAME_H

#include "decoder.h"
#include "frame.hpp"

#include <QAudio>
#include <QAudioFormat>

class QAudioDevice;

namespace Ffmpeg {

class DecoderAudioFrame : public Decoder<FramePtr>
{
    Q_OBJECT
public:
    explicit DecoderAudioFrame(QObject *parent = nullptr);
    ~DecoderAudioFrame();

    void setVolume(qreal volume);

    void setMasterClock();

signals:
    void positionChanged(qint64 position); // microsecond

protected:
    void runDecoder() override;

private:
    class DecoderAudioFramePrivate;
    QScopedPointer<DecoderAudioFramePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // DECODERAUDIOFRAME_H
