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

private slots:
    void onStateChanged(QAudio::State state);
    void onAudioOutputsChanged();

protected:
    void runDecoder() override;

private:
    void buildConnect();

    void checkDefaultAudioOutput(QAudioDevice &audioDevice);

    void writeToDevice(QByteArray &audioBuf);
    auto resetAudioOutput() -> QAudioFormat;

    class DecoderAudioFramePrivate;
    QScopedPointer<DecoderAudioFramePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // DECODERAUDIOFRAME_H
