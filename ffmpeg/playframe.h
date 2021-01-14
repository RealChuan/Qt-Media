#ifndef PLAYFRAME_H
#define PLAYFRAME_H

#include <QObject>

struct AVFrame;

namespace Ffmpeg {

class PlayFrame : public QObject
{
public:
    explicit PlayFrame(QObject *parent = nullptr);
    ~PlayFrame();

    void clear();

    AVFrame* avFrame();

    QImage toImage(int width, int height);

private:
    AVFrame *m_frame;
};

}

#endif // PLAYFRAME_H
