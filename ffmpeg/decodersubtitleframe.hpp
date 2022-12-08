#ifndef DECODERSUBTITLEFRAME_HPP
#define DECODERSUBTITLEFRAME_HPP

#include "decoder.h"
#include "subtitle.h"

namespace Ffmpeg {

class DecoderSubtitleFrame : public Decoder<Subtitle *>
{
public:
    explicit DecoderSubtitleFrame(QObject *parent = nullptr);
    ~DecoderSubtitleFrame();

    void stopDecoder() override;

    void pause(bool state) override;

protected:
    void runDecoder() override;

private:
    void checkPause();
    void checkSeek();

    class DecoderSubtitleFramePrivate;
    QScopedPointer<DecoderSubtitleFramePrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // DECODERSUBTITLEFRAME_HPP
