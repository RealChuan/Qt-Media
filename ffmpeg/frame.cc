#include "frame.hpp"

#include <QImage>

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

Frame::Frame(const QImage &image)
{
    m_frame = av_frame_alloc();
    Q_ASSERT(m_frame != nullptr);
    auto img = image;
    img.convertTo(QImage::Format_RGBA8888);
    m_frame->width = img.width();
    m_frame->height = img.height();
    m_frame->format = AV_PIX_FMT_RGBA;
    av_frame_get_buffer(m_frame, 0);
    memcpy(m_frame->data[0], img.bits(), m_frame->width * m_frame->height * 4);
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

bool Frame::imageAlloc(const QSize &size, AVPixelFormat pix_fmt, int align)
{
    Q_ASSERT(size.isValid());
    m_imageAlloc = true;
    int ret = av_image_alloc(m_frame->data,
                             m_frame->linesize,
                             size.width(),
                             size.height(),
                             pix_fmt,
                             align);
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

void Frame::setPts(double pts)
{
    m_pts = pts;
}

double Frame::pts()
{
    return m_pts;
}

void Frame::setDuration(double duration)
{
    m_duration = duration;
}

double Frame::duration()
{
    return m_duration;
}

bool Frame::isKey()
{
    return m_frame->key_frame == 1;
}

AVFrame *Frame::avFrame()
{
    Q_ASSERT(m_frame != nullptr);
    return m_frame;
}

} // namespace Ffmpeg
