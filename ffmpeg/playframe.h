#ifndef PLAYFRAME_H
#define PLAYFRAME_H

#include <QObject>

struct AVFrame;

namespace Ffmpeg {

class CodecContext;
class PlayFrame : public QObject
{
public:
    explicit PlayFrame(QObject *parent = nullptr);
    ~PlayFrame();

    void clear();

    AVFrame* avFrame();

    QImage toImage(CodecContext *codecContext);

private:
    AVFrame *m_frame;
};

}

#endif // PLAYFRAME_H
