#ifndef VIDEOFORMAT_HPP
#define VIDEOFORMAT_HPP

#include <QImage>
#include <QMap>

extern "C" {
#include <libavutil/pixfmt.h>
}

namespace Ffmpeg {

namespace VideoFormat {

static const QMap<AVPixelFormat, QImage::Format> qFormatMaps = {
    {AV_PIX_FMT_NONE, QImage::Format_Invalid},
    {AV_PIX_FMT_RGB24, QImage::Format_RGB888},
    {AV_PIX_FMT_BGR24, QImage::Format_BGR888},
    {AV_PIX_FMT_RGB32, QImage::Format_RGB32},
    {AV_PIX_FMT_RGBA, QImage::Format_RGBA8888},
    {AV_PIX_FMT_RGB565BE, QImage::Format_RGB16},
    {AV_PIX_FMT_RGB565LE, QImage::Format_RGB16},
    {AV_PIX_FMT_RGB555BE, QImage::Format_RGB555},
    {AV_PIX_FMT_RGB0, QImage::Format_RGBX8888},
    {AV_PIX_FMT_RGBA64BE, QImage::Format_RGBA64},
    {AV_PIX_FMT_RGBA64LE, QImage::Format_RGBA64},
};

}

} // namespace Ffmpeg

#endif // VIDEOFORMAT_HPP
