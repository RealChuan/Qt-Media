// Most of this code comes from mpv

#ifndef COLORUTILS_HPP
#define COLORUTILS_HPP

#include "ffmepg_global.h"

#include <QGenericMatrix>
#include <QObject>
#include <QVector3D>

extern "C" {
#include <libavutil/pixfmt.h>
}

namespace Ffmpeg {

class Frame;

namespace ColorUtils {

struct RawPrimaries
{
    QPointF red = {0, 0}, green = {0, 0}, blue = {0, 0}, white = {0, 0};
};

struct YuvToRgbParam
{
    QVector3D offset;
    QMatrix3x3 matrix;
};

auto getYuvToRgbParam(Frame *frame) -> YuvToRgbParam;

auto getRawPrimaries(AVColorPrimaries color_primaries) -> RawPrimaries;

auto supportConvertColorPrimaries(AVColorPrimaries color_primaries) -> bool;

void getCMSMatrix(const RawPrimaries &src, const RawPrimaries &dest, float m[3][3]);

void getRgb2xyzMatrix(const RawPrimaries &space, float m[3][3]);

void invertMatrix3x3(float m[3][3]);

void mulMatrix3x3(float a[3][3], float b[3][3]);

class FFMPEG_EXPORT Primaries : public QObject
{
    Q_OBJECT
public:
    enum Type { AUTO, BT709, BT470M, BT470BG, SMPTE170M, SMPTE240M, BT2020, SMPTE431, SMPTE432 };
    Q_ENUM(Type);

    using QObject::QObject;

    static auto getAVColorPrimaries(Type type) -> AVColorPrimaries;
};

struct ColorSpaceTrc
{
    auto operator=(const ColorSpaceTrc &other) -> ColorSpaceTrc &;

    auto operator==(const ColorSpaceTrc &other) const -> bool;
    auto operator!=(const ColorSpaceTrc &other) const -> bool;

    const float contrast_min = 0.0;
    const float contrast_max = 2.0;
    const float contrast_default = 1.0;
    float contrast = 1.0;

    const float saturation_min = 0.0;
    const float saturation_max = 2.0;
    const float saturation_default = 1.0;
    float saturation = 1.0;

    const float brightness_min = -1.0;
    const float brightness_max = 1.0;
    const float brightness_default = 0.0;
    float brightness = 0.0;
};

} // namespace ColorUtils

} // namespace Ffmpeg

#endif // COLORUTILS_HPP
