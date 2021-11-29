#ifndef DECODERVIDEOFRAME_H
#define DECODERVIDEOFRAME_H

#include "decoder.h"
#include "playframe.h"

#include <utils/taskqueue.h>

struct AVFrame;

namespace Ffmpeg {

class DecoderVideoFrame : public Decoder<PlayFrame *>
{
    Q_OBJECT
public:
    explicit DecoderVideoFrame(QObject *parent = nullptr);
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

    class DecoderVideoFramePrivate;
    QScopedPointer<DecoderVideoFramePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // DECODERVIDEOFRAME_H
