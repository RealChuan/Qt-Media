#ifndef PLAYFRAME_H
#define PLAYFRAME_H

#include <QObject>

extern "C"{
#include <libavformat/avformat.h>
}

class PlayFrame : public QObject
{
public:
    explicit PlayFrame(QObject *parent = nullptr);
    ~PlayFrame();

    AVFrame* avFrame();

    QImage toImage(int width, int height);

private:
    AVFrame *m_frame;
};

#endif // PLAYFRAME_H
