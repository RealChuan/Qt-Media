#ifndef COLORSPACE_HPP
#define COLORSPACE_HPP

#include <QGenericMatrix>
#include <QVector3D>

namespace Ffmpeg {

class Frame;

namespace ColorSpace {

struct YuvToRgbParam
{
    QVector3D offset;
    QMatrix3x3 matrix;
};

auto getYuvToRgbParam(Frame *frame) -> YuvToRgbParam;

} // namespace ColorSpace

} // namespace Ffmpeg

#endif // COLORSPACE_HPP
