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
    ~PlayFrame();

    void clear();

    AVFrame *avFrame();

    QImage toImage(CodecContext *codecContext);

private:
    Q_DISABLE_COPY(PlayFrame)

    AVFrame *m_frame;
};

} // namespace Ffmpeg

#endif // PLAYFRAME_H
