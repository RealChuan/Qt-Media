// Most of this code comes from mpv

#include "colorutils.hpp"
#include "frame.hpp"

#include <utils/utils.h>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
}

namespace Ffmpeg {

namespace ColorUtils {

static constexpr QVector3D kJPEG_full_offset = {0.000000F, -0.501961F, -0.501961F};
static constexpr QVector3D kBT601_limited_offset = {-0.062745F, -0.501961F, -0.501961F};
static constexpr QVector3D kBT709_full_offset = {0.000000F, -0.501961F, -0.501961F};
static constexpr QVector3D kBT709_limited_offset = {-0.062745F, -0.501961F, -0.501961F};
static constexpr QVector3D kBT2020_8bit_full_offset = {0.000000F, -0.501961F, -0.501961F};
static constexpr QVector3D kBT2020_8bit_limited_offset = {-0.062745F, -0.501961F, -0.501961F};
static constexpr QVector3D kBT2020_10bit_full_offset = {0.000000F, -0.500489F, -0.500489F};
static constexpr QVector3D kBT2020_10bit_limited_offset = {-0.062561F, -0.500489F, -0.500489F};
static constexpr QVector3D kBT2020_12bit_full_offset = {0.000000F, -0.500122F, -0.500122F};
static constexpr QVector3D kBT2020_12bit_limited_offset = {-0.062515F, -0.500122F, -0.500122F};

static constexpr std::array<std::array<float, 3>, 3> kJPEG_full_yuv_to_rgb
    = {std::array<float, 3>{1.000000F, 1.000000F, 1.000000F},
       std::array<float, 3>{-0.000000F, -0.344136F, 1.772000F},
       std::array<float, 3>{1.402000F, -0.714136F, 0.000000F}};
static constexpr std::array<std::array<float, 3>, 3> kRec601_limited_yuv_to_rgb
    = {std::array<float, 3>{1.164384F, 1.164384F, 1.164384F},
       std::array<float, 3>{-0.000000F, -0.391762F, 2.017232F},
       std::array<float, 3>{1.596027F, -0.812968F, 0.000000F}};
static constexpr std::array<std::array<float, 3>, 3> kRec709_full_yuv_to_rgb
    = {std::array<float, 3>{1.000000F, 1.000000F, 1.000000F},
       std::array<float, 3>{-0.000000F, -0.187324F, 1.855600F},
       std::array<float, 3>{1.574800F, -0.468124F, -0.000000F}};
static constexpr std::array<std::array<float, 3>, 3> kRec709_limited_yuv_to_rgb
    = {std::array<float, 3>{1.164384F, 1.164384F, 1.164384F},
       std::array<float, 3>{-0.000000F, -0.213249F, 2.112402F},
       std::array<float, 3>{1.792741F, -0.532909F, -0.000000F}};
static constexpr std::array<std::array<float, 3>, 3> kBT2020_8bit_full_yuv_to_rgb
    = {std::array<float, 3>{1.000000F, 1.000000F, 1.000000F},
       std::array<float, 3>{-0.000000F, -0.164553F, 1.881400F},
       std::array<float, 3>{1.474600F, -0.571353F, -0.000000F}};
static constexpr std::array<std::array<float, 3>, 3> kBT2020_8bit_limited_yuv_to_rgb
    = {std::array<float, 3>{1.164384F, 1.164384F, 1.164384F},
       std::array<float, 3>{-0.000000F, -0.187326F, 2.141772F},
       std::array<float, 3>{1.678674F, -0.650424F, -0.000000F}};
static constexpr std::array<std::array<float, 3>, 3> kBT2020_10bit_full_yuv_to_rgb
    = {std::array<float, 3>{1.000000F, 1.000000F, 1.000000F},
       std::array<float, 3>{-0.000000F, -0.164553F, 1.881400F},
       std::array<float, 3>{1.474600F, -0.571353F, -0.000000F}};
static constexpr std::array<std::array<float, 3>, 3> kBT2020_10bit_limited_yuv_to_rgb
    = {std::array<float, 3>{1.167808F, 1.167808F, 1.167808F},
       std::array<float, 3>{-0.000000F, -0.187877F, 2.148072F},
       std::array<float, 3>{1.683611F, -0.652337F, -0.000000F}};
static constexpr std::array<std::array<float, 3>, 3> kBT2020_12bit_full_yuv_to_rgb
    = {std::array<float, 3>{1.000000F, 1.000000F, 1.000000F},
       std::array<float, 3>{-0.000000F, -0.164553F, 1.881400F},
       std::array<float, 3>{1.474600F, -0.571353F, -0.000000F}};
static constexpr std::array<std::array<float, 3>, 3> kBT2020_12bit_limited_yuv_to_rgb
    = {std::array<float, 3>{1.168664F, 1.168664F, 1.168664F},
       std::array<float, 3>{-0.000000F, -0.188015F, 2.149647F},
       std::array<float, 3>{1.684846F, -0.652816F, -0.000000F}};

auto getYuvToRgbParam(Frame *frame) -> YuvToRgbParam
{
    auto *avFrame = frame->avFrame();
    bool isFullRange = avFrame->color_range == AVCOL_RANGE_JPEG;
    YuvToRgbParam param;
    switch (avFrame->colorspace) {
    case AVCOL_SPC_BT709:
        if (isFullRange) {
            param.offset = kBT709_full_offset;
            param.matrix = QMatrix3x3(kRec709_full_yuv_to_rgb[0].data());
        } else {
            param.offset = kBT709_limited_offset;
            param.matrix = QMatrix3x3(kRec709_limited_yuv_to_rgb[0].data());
        }
        break;
    case AVCOL_SPC_BT2020_NCL: {
        int bitsPerPixel = av_get_bits_per_pixel(
            av_pix_fmt_desc_get(static_cast<AVPixelFormat>(avFrame->format)));
        switch (bitsPerPixel) {
        case 8:
            if (isFullRange) {
                param.offset = kBT2020_8bit_full_offset;
                param.matrix = QMatrix3x3(kBT2020_8bit_full_yuv_to_rgb[0].data());
            } else {
                param.offset = kBT2020_8bit_limited_offset;
                param.matrix = QMatrix3x3(kBT2020_8bit_limited_yuv_to_rgb[0].data());
            }
            break;
        case 12:
            if (isFullRange) {
                param.offset = kBT2020_12bit_full_offset;
                param.matrix = QMatrix3x3(kBT2020_12bit_full_yuv_to_rgb[0].data());
            } else {
                param.offset = kBT2020_12bit_limited_offset;
                param.matrix = QMatrix3x3(kBT2020_12bit_limited_yuv_to_rgb[0].data());
            }
            break;
        default:
            if (isFullRange) {
                param.offset = kBT2020_10bit_full_offset;
                param.matrix = QMatrix3x3(kBT2020_10bit_full_yuv_to_rgb[0].data());
            } else {
                param.offset = kBT2020_10bit_limited_offset;
                param.matrix = QMatrix3x3(kBT2020_10bit_limited_yuv_to_rgb[0].data());
            }
            break;
        }
        break;
    }
    default:
        if (isFullRange) {
            param.offset = kJPEG_full_offset;
            param.matrix = QMatrix3x3(kJPEG_full_yuv_to_rgb[0].data());
        } else {
            param.offset = kBT601_limited_offset;
            param.matrix = QMatrix3x3(kRec601_limited_yuv_to_rgb[0].data());
        }
        break;
    };
    return param;
}

auto getRawPrimaries(AVColorPrimaries color_primaries) -> RawPrimaries
{
    /*
    Values from: ITU-R Recommendations BT.470-6, BT.601-7, BT.709-5, BT.2020-0

    https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.470-6-199811-S!!PDF-E.pdf
    https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.601-7-201103-I!!PDF-E.pdf
    https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.709-5-200204-I!!PDF-E.pdf
    https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.2020-0-201208-I!!PDF-E.pdf

    Other colorspaces from https://en.wikipedia.org/wiki/RGB_color_space#Specifications
    */

    // CIE standard illuminant series
    static constexpr QPointF d50 = {0.34577, 0.35850};
    static constexpr QPointF d65 = {0.31271, 0.32902};
    static constexpr QPointF c = {0.31006, 0.31616};
    static constexpr QPointF dci = {0.31400, 0.35100};
    static constexpr QPointF e = {1.0 / 3.0, 1.0 / 3.0};

    RawPrimaries primaries;
    switch (color_primaries) {
    case AVCOL_PRI_BT709:
        primaries.red = {0.640, 0.330};
        primaries.green = {0.300, 0.600};
        primaries.blue = {0.150, 0.060};
        primaries.white = d65;
        break;
    case AVCOL_PRI_BT470M:
        primaries.red = {0.670, 0.330};
        primaries.green = {0.210, 0.710};
        primaries.blue = {0.140, 0.080};
        primaries.white = d65;
        break;
    case AVCOL_PRI_BT470BG:
        primaries.red = {0.640, 0.330};
        primaries.green = {0.290, 0.600};
        primaries.blue = {0.150, 0.060};
        primaries.white = d65;
        break;
    case AVCOL_PRI_SMPTE170M:
    case AVCOL_PRI_SMPTE240M:
        primaries.red = {0.630, 0.340};
        primaries.green = {0.310, 0.595};
        primaries.blue = {0.155, 0.070};
        primaries.white = d65;
        break;
    case AVCOL_PRI_BT2020:
        primaries.red = {0.708, 0.292};
        primaries.green = {0.170, 0.797};
        primaries.blue = {0.131, 0.046};
        primaries.white = d65;
        break;
    case AVCOL_PRI_SMPTE431:
    case AVCOL_PRI_SMPTE432:
        primaries.red = {0.680, 0.320};
        primaries.green = {0.265, 0.690};
        primaries.blue = {0.150, 0.060};
        primaries.white = color_primaries == AVCOL_PRI_SMPTE431 ? dci : d65;
        break;
    default: break;
    }
    return primaries;
}

auto supportConvertColorPrimaries(AVColorPrimaries color_primaries) -> bool
{
    switch (color_primaries) {
    case AVCOL_PRI_BT709:
    case AVCOL_PRI_BT470M:
    case AVCOL_PRI_BT470BG:
    case AVCOL_PRI_SMPTE170M:
    case AVCOL_PRI_SMPTE240M:
    case AVCOL_PRI_BT2020:
    case AVCOL_PRI_SMPTE431:
    case AVCOL_PRI_SMPTE432: return true;
    default: break;
    }
    return false;
}

void getCMSMatrix(const RawPrimaries &src, const RawPrimaries &dest, float m[3][3])
{
    float tmp[3][3];

    // RGBd<-RGBs = RGBd<-XYZd * XYZd<-XYZs * XYZs<-RGBs
    // Equations from: http://www.brucelindbloom.com/index.html?Math.html
    // Note: Perceptual is treated like relative colorimetric. There's no
    // definition for perceptual other than "make it look good".

    // RGBd<-XYZd, inverted from XYZd<-RGBd
    getRgb2xyzMatrix(dest, m);
    invertMatrix3x3(m);

    // XYZs<-RGBs
    getRgb2xyzMatrix(src, tmp);
    mulMatrix3x3(m, tmp);
}

void getRgb2xyzMatrix(const RawPrimaries &space, float m[3][3])
{
    float S[3], X[4], Z[4];

    // Convert from CIE xyY to XYZ. Note that Y=1 holds true for all primaries
    X[0] = space.red.x() / space.red.y();
    X[1] = space.green.x() / space.green.y();
    X[2] = space.blue.x() / space.blue.y();
    X[3] = space.white.x() / space.white.y();

    Z[0] = (1 - space.red.x() - space.red.y()) / space.red.y();
    Z[1] = (1 - space.green.x() - space.green.y()) / space.green.y();
    Z[2] = (1 - space.blue.x() - space.blue.y()) / space.blue.y();
    Z[3] = (1 - space.white.x() - space.white.y()) / space.white.y();

    // S = XYZ^-1 * W
    for (int i = 0; i < 3; i++) {
        m[0][i] = X[i];
        m[1][i] = 1;
        m[2][i] = Z[i];
    }

    invertMatrix3x3(m);

    for (int i = 0; i < 3; i++) {
        S[i] = m[i][0] * X[3] + m[i][1] * 1 + m[i][2] * Z[3];
    }

    // M = [Sc * XYZc]
    for (int i = 0; i < 3; i++) {
        m[0][i] = S[i] * X[i];
        m[1][i] = S[i] * 1;
        m[2][i] = S[i] * Z[i];
    }
}

void invertMatrix3x3(float m[3][3])
{
    float m00 = m[0][0], m01 = m[0][1], m02 = m[0][2], m10 = m[1][0], m11 = m[1][1], m12 = m[1][2],
          m20 = m[2][0], m21 = m[2][1], m22 = m[2][2];

    // calculate the adjoint
    m[0][0] = (m11 * m22 - m21 * m12);
    m[0][1] = -(m01 * m22 - m21 * m02);
    m[0][2] = (m01 * m12 - m11 * m02);
    m[1][0] = -(m10 * m22 - m20 * m12);
    m[1][1] = (m00 * m22 - m20 * m02);
    m[1][2] = -(m00 * m12 - m10 * m02);
    m[2][0] = (m10 * m21 - m20 * m11);
    m[2][1] = -(m00 * m21 - m20 * m01);
    m[2][2] = (m00 * m11 - m10 * m01);

    // calculate the determinant (as inverse == 1/det * adjoint,
    // adjoint * m == identity * det, so this calculates the det)
    float det = m00 * m[0][0] + m10 * m[0][1] + m20 * m[0][2];
    det = 1.0F / det;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            m[i][j] *= det;
        }
    }
}

// A := A * B
void mulMatrix3x3(float a[3][3], float b[3][3])
{
    float a00 = a[0][0], a01 = a[0][1], a02 = a[0][2], a10 = a[1][0], a11 = a[1][1], a12 = a[1][2],
          a20 = a[2][0], a21 = a[2][1], a22 = a[2][2];

    for (int i = 0; i < 3; i++) {
        a[0][i] = a00 * b[0][i] + a01 * b[1][i] + a02 * b[2][i];
        a[1][i] = a10 * b[0][i] + a11 * b[1][i] + a12 * b[2][i];
        a[2][i] = a20 * b[0][i] + a21 * b[1][i] + a22 * b[2][i];
    }
}

auto Primaries::getAVColorPrimaries(Type type) -> AVColorPrimaries
{
    switch (type) {
    case AUTO:
    case BT709: return AVCOL_PRI_BT709;
    case BT470M: return AVCOL_PRI_BT470M;
    case BT470BG: return AVCOL_PRI_BT470BG;
    case SMPTE170M: return AVCOL_PRI_SMPTE170M;
    case SMPTE240M: return AVCOL_PRI_SMPTE240M;
    case BT2020: return AVCOL_PRI_BT2020;
    case SMPTE431: return AVCOL_PRI_SMPTE431;
    case SMPTE432: return AVCOL_PRI_SMPTE432;
    default: break;
    }
    return AVCOL_PRI_RESERVED0;
}

} // namespace ColorUtils

} // namespace Ffmpeg
