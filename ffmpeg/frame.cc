#include "frame.hpp"
#include "averrormanager.hpp"

#include <QImage>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

namespace Ffmpeg {

struct Frame::FramePrivate
{
    ~FramePrivate() { reset(); }

    void reset()
    {
        pts = 0;
        duration = 0;
        format = QImage::Format_Invalid;
        Q_ASSERT(frame != nullptr);
        freeImageAlloc();
        av_frame_free(&frame);
    }

    void freeImageAlloc()
    {
        if (imageAlloc) {
            av_freep(&frame->data[0]);
            imageAlloc = false;
        }
    }

    void setError(int errorCode) { AVErrorManager::instance()->setErrorCode(errorCode); }

    AVFrame *frame;
    bool imageAlloc = false;
    double pts = 0;
    double duration = 0;

    QImage::Format format = QImage::Format_Invalid;

    bool useNull = false;
};

Frame::Frame()
    : d_ptr(new FramePrivate)
{
    d_ptr->frame = av_frame_alloc();
    Q_ASSERT(d_ptr->frame != nullptr);
}

Frame::Frame(const QImage &image)
    : d_ptr(new FramePrivate)
{
    d_ptr->frame = av_frame_alloc();
    Q_ASSERT(d_ptr->frame != nullptr);
    auto img = image;
    img.convertTo(QImage::Format_RGBA8888);
    d_ptr->frame->width = img.width();
    d_ptr->frame->height = img.height();
    d_ptr->frame->format = AV_PIX_FMT_RGBA;
    av_frame_get_buffer(d_ptr->frame, 0);
    memcpy(d_ptr->frame->data[0], img.bits(), d_ptr->frame->width * d_ptr->frame->height * 4);
}

Frame::Frame(const Frame &other)
    : d_ptr(new FramePrivate)
{
    d_ptr->frame = av_frame_clone(other.d_ptr->frame);
    d_ptr->pts = other.d_ptr->pts;
    d_ptr->duration = other.d_ptr->duration;
    d_ptr->format = other.d_ptr->format;
    d_ptr->useNull = other.d_ptr->useNull;
}

Frame &Frame::operator=(const Frame &other)
{
    d_ptr->reset();
    d_ptr->frame = av_frame_clone(other.d_ptr->frame);
    d_ptr->pts = other.d_ptr->pts;
    d_ptr->duration = other.d_ptr->duration;
    d_ptr->format = other.d_ptr->format;
    d_ptr->useNull = other.d_ptr->useNull;
    return *this;
}

Frame::~Frame() {}

void Frame::copyPropsFrom(Frame *src)
{
    auto srcFrame = src->avFrame();
    av_frame_copy_props(d_ptr->frame, srcFrame);
    d_ptr->frame->width = srcFrame->width;
    d_ptr->frame->height = srcFrame->height;
    d_ptr->frame->pts = srcFrame->pts;
    d_ptr->frame->pkt_dts = srcFrame->pkt_dts;
    d_ptr->frame->key_frame = srcFrame->key_frame;
}

bool Frame::imageAlloc(const QSize &size, AVPixelFormat pix_fmt, int align)
{
    Q_ASSERT(size.isValid());
    d_ptr->imageAlloc = true;
    int ret = av_image_alloc(d_ptr->frame->data,
                             d_ptr->frame->linesize,
                             size.width(),
                             size.height(),
                             pix_fmt,
                             align);
    return ret >= 0;
}

void Frame::freeImageAlloc()
{
    d_ptr->freeImageAlloc();
}

void Frame::setPictType(AVPictureType type)
{
    d_ptr->frame->pict_type = type;
}

void Frame::unref()
{
    av_frame_unref(d_ptr->frame);
}

void Frame::setPts(double pts)
{
    d_ptr->pts = pts;
}

double Frame::pts()
{
    return d_ptr->pts;
}

void Frame::setDuration(double duration)
{
    d_ptr->duration = duration;
}

double Frame::duration()
{
    return d_ptr->duration;
}

int Frame::format() const
{
    return d_ptr->frame->format;
}

void Frame::setAVFrameNull()
{
    d_ptr->useNull = true;
}

void Frame::setQImageFormat(QImage::Format format)
{
    d_ptr->format = format;
}

QImage::Format Frame::qImageformat() const
{
    return d_ptr->format;
}

QImage Frame::convertToImage() const
{
    if (d_ptr->format == QImage::Format_Invalid) {
        return QImage();
    }

    return QImage((uchar *) d_ptr->frame->data[0],
                  d_ptr->frame->width,
                  d_ptr->frame->height,
                  d_ptr->frame->linesize[0],
                  d_ptr->format);
}

bool Frame::getBuffer()
{
    auto ret = av_frame_get_buffer(d_ptr->frame, 0);
    if (ret < 0) {
        d_ptr->setError(ret);
        return false;
    }
    return true;
}

bool Frame::isKey()
{
    return d_ptr->frame->key_frame == 1;
}

AVFrame *Frame::avFrame()
{
    if (d_ptr->useNull) {
        return nullptr;
    }
    Q_ASSERT(d_ptr->frame != nullptr);
    return d_ptr->frame;
}

} // namespace Ffmpeg
