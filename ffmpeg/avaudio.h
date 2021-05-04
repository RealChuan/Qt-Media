#ifndef AVAUDIO_H
#define AVAUDIO_H

#include <QtCore>

extern "C"{
#include <libavutil/samplefmt.h>
}

struct SwrContext;

namespace Ffmpeg {

class CodecContext;
class PlayFrame;
class AVAudio
{
public:
    AVAudio(CodecContext *codecCtx, AVSampleFormat format);
    ~AVAudio();

    QByteArray convert(PlayFrame *frame);

private:
    SwrContext *m_swrContext;
    AVSampleFormat m_format;
};

}

#endif // AVAUDIO_H
