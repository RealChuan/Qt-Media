#include "frame.hpp"

#include <QtGlobal>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

Frame::Frame()
{
    m_frame = av_frame_alloc();
    Q_ASSERT(m_frame != nullptr);
}

Frame::Frame(const Frame &other)
{
    m_frame = av_frame_clone(other.m_frame);
}

Frame &Frame::operator=(const Frame &other)
{
    m_frame = av_frame_clone(other.m_frame);
    return *this;
}

Frame::~Frame()
{
    Q_ASSERT(m_frame != nullptr);
    av_frame_free(&m_frame);
}

void Frame::clear()
{
    av_frame_unref(m_frame);
}

AVFrame *Frame::avFrame()
{
    Q_ASSERT(m_frame != nullptr);
    return m_frame;
}

} // namespace Ffmpeg
