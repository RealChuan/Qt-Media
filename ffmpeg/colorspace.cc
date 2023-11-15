#include "colorspace.hpp"
#include "frame.hpp"

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
}

namespace Ffmpeg::ColorSpace {

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

static constexpr float kJPEG_full_yuv_to_rgb[3][3] = {{1.000000F, 1.000000F, 1.000000F},
                                                      {-0.000000F, -0.344136F, 1.772000F},
                                                      {1.402000F, -0.714136F, 0.000000F}};
static constexpr float kRec601_limited_yuv_to_rgb[3][3] = {{1.164384F, 1.164384F, 1.164384F},
                                                           {-0.000000F, -0.391762F, 2.017232F},
                                                           {1.596027F, -0.812968F, 0.000000F}};
static constexpr float kRec709_full_yuv_to_rgb[3][3] = {{1.000000F, 1.000000F, 1.000000F},
                                                        {-0.000000F, -0.187324F, 1.855600F},
                                                        {1.574800F, -0.468124F, -0.000000F}};
static constexpr float kRec709_limited_yuv_to_rgb[3][3] = {{1.164384F, 1.164384F, 1.164384F},
                                                           {-0.000000F, -0.213249F, 2.112402F},
                                                           {1.792741F, -0.532909F, -0.000000F}};
static constexpr float kBT2020_8bit_full_yuv_to_rgb[3][3] = {{1.000000F, 1.000000F, 1.000000F},
                                                             {-0.000000F, -0.164553F, 1.881400F},
                                                             {1.474600F, -0.571353F, -0.000000F}};
static constexpr float kBT2020_8bit_limited_yuv_to_rgb[3][3] = {{1.164384F, 1.164384F, 1.164384F},
                                                                {-0.000000F, -0.187326F, 2.141772F},
                                                                {1.678674F, -0.650424F, -0.000000F}};
static constexpr float kBT2020_10bit_full_yuv_to_rgb[3][3] = {{1.000000F, 1.000000F, 1.000000F},
                                                              {-0.000000F, -0.164553F, 1.881400F},
                                                              {1.474600F, -0.571353F, -0.000000F}};
static constexpr float kBT2020_10bit_limited_yuv_to_rgb[3][3]
    = {{1.167808F, 1.167808F, 1.167808F},
       {-0.000000F, -0.187877F, 2.148072F},
       {1.683611F, -0.652337F, -0.000000F}};
static constexpr float kBT2020_12bit_full_yuv_to_rgb[3][3] = {{1.000000F, 1.000000F, 1.000000F},
                                                              {-0.000000F, -0.164553F, 1.881400F},
                                                              {1.474600F, -0.571353F, -0.000000F}};
static constexpr float kBT2020_12bit_limited_yuv_to_rgb[3][3]
    = {{1.168664F, 1.168664F, 1.168664F},
       {-0.000000F, -0.188015F, 2.149647F},
       {1.684846F, -0.652816F, -0.000000F}};

auto getYuvToRgbParam(Frame *frame) -> YuvToRgbParam
{
    auto *avFrame = frame->avFrame();
    bool isFullRange = avFrame->color_range == AVCOL_RANGE_JPEG;
    YuvToRgbParam param;
    switch (avFrame->colorspace) {
    case AVCOL_SPC_BT709:
        if (isFullRange) {
            param.offset = kBT709_full_offset;
            param.matrix = QMatrix3x3(&kRec709_full_yuv_to_rgb[0][0]);
        } else {
            param.offset = kBT709_limited_offset;
            param.matrix = QMatrix3x3(&kRec709_limited_yuv_to_rgb[0][0]);
        }
        break;
    case AVCOL_SPC_BT2020_NCL: {
        int bitsPerPixel = av_get_bits_per_pixel(
            av_pix_fmt_desc_get(static_cast<AVPixelFormat>(avFrame->format)));
        switch (bitsPerPixel) {
        case 8:
            if (isFullRange) {
                param.offset = kBT2020_8bit_full_offset;
                param.matrix = QMatrix3x3(&kBT2020_8bit_full_yuv_to_rgb[0][0]);
            } else {
                param.offset = kBT2020_8bit_limited_offset;
                param.matrix = QMatrix3x3(&kBT2020_8bit_limited_yuv_to_rgb[0][0]);
            }
            break;
        case 12:
            if (isFullRange) {
                param.offset = kBT2020_12bit_full_offset;
                param.matrix = QMatrix3x3(&kBT2020_12bit_full_yuv_to_rgb[0][0]);
            } else {
                param.offset = kBT2020_12bit_limited_offset;
                param.matrix = QMatrix3x3(&kBT2020_12bit_limited_yuv_to_rgb[0][0]);
            }
            break;
        default:
            if (isFullRange) {
                param.offset = kBT2020_10bit_full_offset;
                param.matrix = QMatrix3x3(&kBT2020_10bit_full_yuv_to_rgb[0][0]);
            } else {
                param.offset = kBT2020_10bit_limited_offset;
                param.matrix = QMatrix3x3(&kBT2020_10bit_limited_yuv_to_rgb[0][0]);
            }
            break;
        }
        break;
    }
    default:
        if (isFullRange) {
            param.offset = kJPEG_full_offset;
            param.matrix = QMatrix3x3(&kJPEG_full_yuv_to_rgb[0][0]);
        } else {
            param.offset = kBT601_limited_offset;
            param.matrix = QMatrix3x3(&kRec601_limited_yuv_to_rgb[0][0]);
        }
        break;
    };
    return param;
}

} // namespace Ffmpeg::ColorSpace
