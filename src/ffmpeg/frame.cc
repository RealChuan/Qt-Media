#include "frame.hpp"
#include "averrormanager.hpp"
#include "ffmpegutils.hpp"
#include "videoformat.hpp"

#include <QImage>

extern "C" {
#include <libavutil/imgutils.h>
}

namespace Ffmpeg {

struct AVFrameDeleter
{
    void operator()(AVFrame *f) const noexcept { av_frame_free(&f); }
};
using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;

class Frame::FramePrivate
{
public:
    explicit FramePrivate()
        : frame(av_frame_alloc())
    {
        if (!frame) {
            throw std::bad_alloc();
        }
    }

    ~FramePrivate() { destroyFrame(); }

    void destroyFrame()
    {
        freeImageAlloc();
        frame.reset();
    }

    void freeImageAlloc()
    {
        if (imageAlloc) {
            av_freep(&frame->data[0]);
            imageAlloc = false;
        }
    }

    AVFramePtr frame;
    bool imageAlloc = false;
};

Frame::Frame()
    : d_ptr(std::make_unique<FramePrivate>())
{}

Frame::~Frame() = default;

Frame::Frame(const Frame &other)
    : d_ptr(std::make_unique<FramePrivate>())
{
    av_frame_ref(d_ptr->frame.get(), other.d_ptr->frame.get());
}

Frame::Frame(Frame &&other) noexcept = default;

Frame &Frame::operator=(const Frame &other)
{
    if (this != &other) {
        d_ptr->freeImageAlloc();
        av_frame_unref(d_ptr->frame.get());
        av_frame_ref(d_ptr->frame.get(), other.d_ptr->frame.get());
    }
    return *this;
}

Frame &Frame::operator=(Frame &&other) noexcept = default;

bool Frame::compareProps(Frame *other)
{
    Q_ASSERT(other && d_ptr->frame);
    auto *of = other->avFrame();
    return d_ptr->frame->width == of->width && d_ptr->frame->height == of->height
           && d_ptr->frame->format == of->format
           && compareAVRational(d_ptr->frame->time_base, of->time_base)
           && compareAVRational(d_ptr->frame->sample_aspect_ratio, of->sample_aspect_ratio)
           && d_ptr->frame->sample_rate == of->sample_rate
           && av_channel_layout_compare(&d_ptr->frame->ch_layout, &of->ch_layout) == 0;
}

void Frame::copyPropsFrom(Frame *src)
{
    Q_ASSERT(src && d_ptr->frame);
    auto *sf = src->avFrame();
    av_frame_copy_props(d_ptr->frame.get(), sf);
    d_ptr->frame->pts = sf->pts;
    d_ptr->frame->duration = sf->duration;
    d_ptr->frame->pict_type = sf->pict_type;
    d_ptr->frame->flags = sf->flags;
    d_ptr->frame->width = sf->width;
    d_ptr->frame->height = sf->height;
    d_ptr->frame->time_base = sf->time_base;
    d_ptr->frame->sample_aspect_ratio = sf->sample_aspect_ratio;
    d_ptr->frame->sample_rate = sf->sample_rate;
    d_ptr->frame->ch_layout = sf->ch_layout;
}

bool Frame::imageAlloc(const QSize &size, AVPixelFormat pix_fmt, int align)
{
    Q_ASSERT(d_ptr->frame && size.isValid() && pix_fmt != AV_PIX_FMT_NONE);
    d_ptr->freeImageAlloc();

    int ret = av_image_alloc(d_ptr->frame->data,
                             d_ptr->frame->linesize,
                             size.width(),
                             size.height(),
                             pix_fmt,
                             align);
    if (ret < 0) {
        SET_ERROR_CODE(ret);
        return false;
    }
    d_ptr->imageAlloc = true;
    return true;
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
    av_frame_unref(d_ptr->frame.get());
}

void Frame::setPts(qint64 pts)
{
    d_ptr->frame->pts = pts;
}

auto Frame::pts() -> qint64
{
    return d_ptr->frame->pts;
}

void Frame::setDuration(qint64 duration)
{
    d_ptr->frame->duration = duration;
}

auto Frame::duration() -> qint64
{
    return d_ptr->frame->duration;
}

void Frame::destroyFrame()
{
    d_ptr->destroyFrame();
}

auto Frame::toImage() -> QImage
{
    auto *f = d_ptr->frame.get();
    if (!f || !f->data[0] || f->width <= 0 || f->height <= 0 || f->format == AV_PIX_FMT_NONE)
        return {};

    QImage::Format fmt = VideoFormat::qFormatMaps.value(static_cast<AVPixelFormat>(f->format),
                                                        QImage::Format_Invalid);
    if (fmt == QImage::Format_Invalid)
        return {};

    return QImage(f->data[0], f->width, f->height, f->linesize[0], fmt);
}

auto Frame::getBuffer() -> bool
{
    int ret = av_frame_get_buffer(d_ptr->frame.get(), 0);
    ERROR_RETURN(ret)
}

auto Frame::isKey() -> bool
{
    return d_ptr->frame->flags & AV_FRAME_FLAG_KEY;
}

auto Frame::avFrame() -> AVFrame *
{
    return d_ptr->frame.get();
}

auto Frame::fromQImage(const QImage &image) -> Frame *
{
    if (image.isNull() || image.format() == QImage::Format_Invalid)
        return nullptr;

    auto ptr = std::make_unique<Frame>();
    auto *f = ptr->avFrame();
    QImage img = image.convertToFormat(QImage::Format_RGBA8888);

    f->width = img.width();
    f->height = img.height();
    f->format = AV_PIX_FMT_RGBA;

    if (ptr->getBuffer() == false)
        return nullptr;

    memcpy(f->data[0], img.constBits(), static_cast<size_t>(img.height()) * f->linesize[0]);
    ptr->d_ptr->imageAlloc = true; // 标记内存由 av_malloc 分配
    return ptr.release();
}

} // namespace Ffmpeg
