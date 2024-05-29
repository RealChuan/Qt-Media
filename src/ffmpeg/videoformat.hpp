#ifndef VIDEOFORMAT_HPP
#define VIDEOFORMAT_HPP

#include <QImage>
#include <QMap>

extern "C" {
#include <libavutil/pixfmt.h>
}

namespace Ffmpeg {

namespace VideoFormat {

// AVPixelFormat convert to QImage::Format conversion table
static const QMap<AVPixelFormat, QImage::Format> qFormatMaps
    = {{AV_PIX_FMT_NONE, QImage::Format_Invalid},
       {AV_PIX_FMT_RGB24, QImage::Format_RGB888},
       {AV_PIX_FMT_BGR24, QImage::Format_RGB888},
       {AV_PIX_FMT_GRAY8, QImage::Format_Grayscale8},
       {AV_PIX_FMT_MONOWHITE, QImage::Format_Mono},
       {AV_PIX_FMT_MONOBLACK, QImage::Format_Mono},
       {AV_PIX_FMT_PAL8, QImage::Format_Indexed8},
       {AV_PIX_FMT_ARGB, QImage::Format_ARGB32},
       {AV_PIX_FMT_RGBA, QImage::Format_RGBA8888},
       {AV_PIX_FMT_ABGR, QImage::Format_RGB32},
       {AV_PIX_FMT_BGRA, QImage::Format_RGB32},
       {AV_PIX_FMT_GRAY16BE, QImage::Format_Grayscale16},
       {AV_PIX_FMT_GRAY16LE, QImage::Format_Grayscale16},
       {AV_PIX_FMT_RGB565BE, QImage::Format_RGB16},
       {AV_PIX_FMT_RGB565LE, QImage::Format_RGB16},
       {AV_PIX_FMT_RGB555BE, QImage::Format_RGB16},
       {AV_PIX_FMT_RGB555LE, QImage::Format_RGB16},
       {AV_PIX_FMT_BGR565BE, QImage::Format_RGB16},
       {AV_PIX_FMT_BGR565LE, QImage::Format_RGB16},
       {AV_PIX_FMT_BGR555BE, QImage::Format_RGB16},
       {AV_PIX_FMT_BGR555LE, QImage::Format_RGB16},
       {AV_PIX_FMT_RGB0, QImage::Format_RGB32},
       {AV_PIX_FMT_BGR0, QImage::Format_RGB32},
       {AV_PIX_FMT_0RGB, QImage::Format_RGB32},
       {AV_PIX_FMT_0BGR, QImage::Format_RGB32},
       {AV_PIX_FMT_RGBA64BE, QImage::Format_RGBA64},
       {AV_PIX_FMT_RGBA64LE, QImage::Format_RGBA64},
       {AV_PIX_FMT_BGRA64BE, QImage::Format_RGBA64},
       {AV_PIX_FMT_BGRA64LE, QImage::Format_RGBA64}};

} // namespace VideoFormat

} // namespace Ffmpeg

#endif // VIDEOFORMAT_HPP
