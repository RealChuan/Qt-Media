#include "playframe.h"

#include <QImage>

extern "C"{
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

PlayFrame::PlayFrame(QObject *parent)
    : QObject(parent)
{
    m_frame = av_frame_alloc();
    Q_ASSERT(m_frame != nullptr);
}

PlayFrame::~PlayFrame()
{
    Q_ASSERT(m_frame != nullptr);
    av_frame_free(&m_frame);
}

void PlayFrame::clear()
{
    av_frame_unref(m_frame);
}

AVFrame *PlayFrame::avFrame()
{
    Q_ASSERT(m_frame != nullptr);
    return m_frame;
}

QImage PlayFrame::toImage(int width, int height)
{
    Q_ASSERT(m_frame != nullptr);
    return QImage((uchar*)m_frame->data[0], width, height, QImage::Format_RGB32);
}

}
