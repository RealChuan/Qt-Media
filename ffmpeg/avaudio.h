#ifndef AVAUDIO_H
#define AVAUDIO_H

#include <QtCore>

struct SwrContext;

namespace Ffmpeg {

class CodecContext;
class PlayFrame;
class AVAudio
{
public:
    AVAudio(CodecContext *codecCtx);
    ~AVAudio();

    QByteArray convert(PlayFrame *frame, CodecContext *codecCtx);

private:
    SwrContext *m_swrContext;
};

}

#endif // AVAUDIO_H
