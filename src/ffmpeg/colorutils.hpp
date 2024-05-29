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

struct FFMPEG_EXPORT ColorSpaceTrc
{
    ColorSpaceTrc();
    ColorSpaceTrc(const ColorSpaceTrc &other);
    auto operator=(const ColorSpaceTrc &other) -> ColorSpaceTrc &;
    ~ColorSpaceTrc() = default;

    auto operator==(const ColorSpaceTrc &other) const -> bool;
    auto operator!=(const ColorSpaceTrc &other) const -> bool;

    void setContrast(float contrast);
    [[nodiscard]] auto contrast() const -> float { return m_contrast; }
    [[nodiscard]] auto eqContrast() const -> float;

    void setSaturation(float saturation);
    [[nodiscard]] auto saturation() const -> float { return m_saturation; }
    [[nodiscard]] auto eqSaturation() const -> float;

    void setBrightness(float brightness);
    [[nodiscard]] auto brightness() const -> float { return m_brightness; }
    [[nodiscard]] auto eqBrightness() const -> float;

    static const float contrast_min;
    static const float contrast_max;
    static const float contrast_default;

    static const float saturation_min;
    static const float saturation_max;
    static const float saturation_default;

    static const float brightness_min;
    static const float brightness_max;
    static const float brightness_default;

private:
    float m_contrast;
    float m_saturation;
    float m_brightness;
};

} // namespace ColorUtils

} // namespace Ffmpeg

#endif // COLORUTILS_HPP
