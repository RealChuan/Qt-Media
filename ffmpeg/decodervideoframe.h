#ifndef DECODERVIDEOFRAME_H
#define DECODERVIDEOFRAME_H

#include "decoder.h"
#include "playframe.h"

struct AVFrame;

namespace Ffmpeg {

class DecoderVideoFramePrivate;
class DecoderVideoFrame : public Decoder<PlayFrame>
{
    Q_OBJECT
public:
    DecoderVideoFrame(QObject *parent = nullptr);
    ~DecoderVideoFrame();

    void stopDecoder() override;

    void pause(bool state) override;

signals:
    void readyRead(const QImage &image);

protected:
    void runDecoder() override;

private:
    void checkPause();
    void checkSeek();

    QScopedPointer<DecoderVideoFramePrivate> d_ptr;
};

}

#endif // DECODERVIDEOFRAME_H
