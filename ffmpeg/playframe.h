#ifndef PLAYFRAME_H
#define PLAYFRAME_H

#include <QImage>

struct AVFrame;

namespace Ffmpeg {

class CodecContext;
class PlayFrame
{
public:
    explicit PlayFrame();
    PlayFrame(const PlayFrame &other);
    PlayFrame &operator=(const PlayFrame &other);
    ~PlayFrame();

    void clear();

    AVFrame *avFrame();

private:
    AVFrame *m_frame;
};

} // namespace Ffmpeg

#endif // PLAYFRAME_H
