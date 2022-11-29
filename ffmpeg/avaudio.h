#ifndef AVAUDIO_H
#define AVAUDIO_H

#include <QtCore>

extern "C" {
#include <libavutil/samplefmt.h>
}

struct SwrContext;

namespace Ffmpeg {

class CodecContext;
class Frame;
class AVAudio
{
public:
    explicit AVAudio(CodecContext *codecCtx, AVSampleFormat format);
    ~AVAudio();

    QByteArray convert(Frame *frame);

private:
    Q_DISABLE_COPY(AVAudio)

    SwrContext *m_swrContext;
    AVSampleFormat m_format;
};

} // namespace Ffmpeg

#endif // AVAUDIO_H
