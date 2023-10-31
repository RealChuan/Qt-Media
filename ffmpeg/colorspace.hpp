#ifndef COLORSPACE_HPP
#define COLORSPACE_HPP

#include <QVector3D>

#include <array>

namespace Ffmpeg::ColorSpace {

static constexpr QVector3D kJPEGOffset = {0, -0.501960814, -0.501960814};

static constexpr std::array<float, 9> kJPEGMatrix
    = {1, 1, 1, 0.000, -0.3441, 1.772, 1.402, -0.7141, 0.000};

static constexpr QVector3D kBT601Offset = {-0.0627451017, -0.501960814, -0.501960814};

static constexpr std::array<float, 9> kBT601Matrix
    = {1.1644, 1.1644, 1.1644, 0.000, -0.3918, 2.0172, 1.596, -0.813, 0.000};

static constexpr QVector3D kBT7090ffset = {-0.0627451017, -0.501960814, -0.501960814};

static constexpr std::array<float, 9> kBT709Matrix
    = {1.1644, 1.1644, 1.1644, 0.000, -0.2132, 2.112, 1.7927, -0.5329, 0.000};

static constexpr QVector3D kBT2020ffset = {-0.0627451017, -0.501960814, -0.501960814};

static constexpr std::array<float, 9> kBT2020Matrix
    = {1.1678, 1.1678, 1.1678, 0.0000, -0.1879, 2.1481, 1.6836, -0.6523, 0.0000};

} // namespace Ffmpeg::ColorSpace

#endif // COLORSPACE_HPP
