#ifndef COLORSPACE_HPP
#define COLORSPACE_HPP

#include <QVector3D>

namespace Ffmpeg {

namespace ColorSpace {

static const QVector3D kJPEGOffset = {0, -0.501960814, -0.501960814};

static const float kJPEGMatrix[] = {1, 1, 1, 0.000, -0.3441, 1.772, 1.402, -0.7141, 0.000};

static const QVector3D kBT601Offset = {-0.0627451017, -0.501960814, -0.501960814};

static const float kBT601Matrix[]
    = {1.1644, 1.1644, 1.1644, 0.000, -0.3918, 2.0172, 1.596, -0.813, 0.000};

static const QVector3D kBT7090ffset = {-0.0627451017, -0.501960814, -0.501960814};

static const float kBT709Matrix[]
    = {1.1644, 1.1644, 1.1644, 0.000, -0.2132, 2.112, 1.7927, -0.5329, 0.000};

} // namespace ColorSpace

} // namespace Ffmpeg

#endif // COLORSPACE_HPP
