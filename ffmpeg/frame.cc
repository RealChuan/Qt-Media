#include "frame.hpp"

#include <QSize>
#include <QtGlobal>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
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
    freeimageAlloc();
    av_frame_free(&m_frame);
}

bool Frame::imageAlloc(const QSize &size, AVPixelFormat pix_fmt)
{
    Q_ASSERT(size.isValid());
    m_imageAlloc = true;
    int ret
        = av_image_alloc(m_frame->data, m_frame->linesize, size.width(), size.height(), pix_fmt, 1);
    return ret >= 0;
}

void Frame::freeimageAlloc()
{
    if (m_imageAlloc) {
        av_freep(&m_frame->data[0]);
        m_imageAlloc = false;
    }
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
