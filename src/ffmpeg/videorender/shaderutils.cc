// Most of this code comes from mpv

#include "shaderutils.hpp"

#include <ffmpeg/colorutils.hpp>
#include <utils/utils.h>

namespace Ffmpeg::ShaderUtils {

// Common constants for SMPTE ST.2084 (HDR)
static constexpr float PQ_M1 = 2610. / 4096 * 1. / 4, PQ_M2 = 2523. / 4096 * 128,
                       PQ_C1 = 3424. / 4096, PQ_C2 = 2413. / 4096 * 32, PQ_C3 = 2392. / 4096 * 32;
// Common constants for ARIB STD-B67 (HLG)
static constexpr float HLG_A = 0.17883277F, HLG_B = 0.28466892F, HLG_C = 0.55991073F;

static constexpr float SMPTEST2048_REF_WHITE = 10000.0;
static constexpr float ARIB_STD_B67_VALUE = 12.0;

auto trcNomPeak(AVColorTransferCharacteristic colortTrc) -> float
{
    switch (colortTrc) {
    case AVCOL_TRC_SMPTEST2084: return static_cast<float>(SMPTEST2048_REF_WHITE / MP_REF_WHITE);
    case AVCOL_TRC_ARIB_STD_B67: return static_cast<float>(ARIB_STD_B67_VALUE / MP_REF_WHITE_HLG);
    default: break;
    }
    return 1.0;
}

auto trcIsHdr(AVColorTransferCharacteristic colortTrc) -> bool
{
    return trcNomPeak(colortTrc) > 1.0;
}

auto header() -> QByteArray
{
    auto header = Utils::readAllFile(":/shader/video_header.frag");
    header.append(GLSL(\n));
    header.append(Utils::readAllFile(":/shader/video_color.frag"));
    header.append(GLSL(\n));
    return header;
}

auto beginFragment(QByteArray &frag, int format) -> bool
{
    switch (format) {
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUV422P:
    case AV_PIX_FMT_YUV444P:
    case AV_PIX_FMT_YUV410P:
    case AV_PIX_FMT_YUV411P: frag.append(Utils::readAllFile(":/shader/video_yuv420p.frag")); break;
    case AV_PIX_FMT_YUYV422: frag.append(Utils::readAllFile(":/shader/video_yuyv422.frag")); break;
    case AV_PIX_FMT_RGB24:
    case AV_PIX_FMT_BGR8:
    case AV_PIX_FMT_RGB8:
        frag.append(GLSL(vec4 color = vec4(0.0, 0.0, 0.0, 1.0);\n));
        frag.append(GLSL(color.rgb = texture(tex_y, TexCord).rgb;\n));
        break;
    case AV_PIX_FMT_BGR24:
        frag.append(GLSL(vec4 color = vec4(0.0, 0.0, 0.0, 1.0);\n));
        frag.append(GLSL(color.rgb = texture(tex_y, TexCord).bgr;\n));
        break;
    case AV_PIX_FMT_UYVY422: frag.append(Utils::readAllFile(":/shader/video_uyvy422.frag")); break;
    case AV_PIX_FMT_NV12:
    case AV_PIX_FMT_P010LE: frag.append(Utils::readAllFile(":/shader/video_nv12.frag")); break;
    case AV_PIX_FMT_NV21: frag.append(Utils::readAllFile(":/shader/video_nv21.frag")); break;
    case AV_PIX_FMT_ARGB: frag.append(GLSL(vec4 color = texture(tex_y, TexCord).gbar;\n)); break;
    case AV_PIX_FMT_RGBA: frag.append(GLSL(vec4 color = texture(tex_y, TexCord).rgba;\n)); break;
    case AV_PIX_FMT_ABGR: frag.append(GLSL(vec4 color = texture(tex_y, TexCord).abgr;\n)); break;
    case AV_PIX_FMT_BGRA: frag.append(GLSL(vec4 color = texture(tex_y, TexCord).bgra;\n)); break;
    default: qWarning() << "UnSupported format:" << format; return false;
    }
    frag.append(GLSL(\n));
    return true;
}

void passLinearize(QByteArray &frag, AVColorTransferCharacteristic colortTrc)
{
    if (colortTrc == AVCOL_TRC_LINEAR) {
        return;
    }
    frag.append("\n// pass linearize\n");
    frag.append(GLSL(color.rgb = clamp(color.rgb, 0.0, 1.0);\n));
    switch (colortTrc) {
    case AVCOL_TRC_BT709:
    case AVCOL_TRC_SMPTE170M:
    case AVCOL_TRC_SMPTE240M:
    case AVCOL_TRC_BT1361_ECG:
    case AVCOL_TRC_BT2020_10:
    case AVCOL_TRC_BT2020_12: frag.append(GLSL(color.rgb = pow(color.rgb, vec3(2.4));\n)); break;
    case AVCOL_TRC_GAMMA22: frag.append(GLSL(color.rgb = pow(color.rgb, vec3(2.2));\n)); break;
    case AVCOL_TRC_GAMMA28: frag.append(GLSL(color.rgb = pow(color.rgb, vec3(2.8));\n)); break;
    case AVCOL_TRC_IEC61966_2_1:
        frag.append("color.rgb = mix(color.rgb * vec3(1.0/12.92), \n"
                    "               pow((color.rgb + vec3(0.055))/vec3(1.055), vec3(2.4)), \n"
                    "               vec3(lessThan(vec3(0.04045), color.rgb))); \n");
        break;
    case AVCOL_TRC_SMPTEST2084: {
        auto temp = QString("color.rgb = pow(color.rgb, vec3(1.0/%1));\n").arg(PQ_M2);
        temp.append(
            QString("color.rgb = max(color.rgb - vec3(%1), vec3(0.0)) \n"
                    "             / (vec3(%2) - vec3(%3) * color.rgb);\n")
                .arg(QString::number(PQ_C1), QString::number(PQ_C2), QString::number(PQ_C3)));
        temp.append(QString("color.rgb = pow(color.rgb, vec3(%1));\n").arg(1.0 / PQ_M1));
        // PQ's output range is 0-10000, but we need it to be relative to
        // MP_REF_WHITE instead, so rescale
        temp.append(QString("color.rgb *= vec3(%1);\n").arg(SMPTEST2048_REF_WHITE / MP_REF_WHITE));
        frag.append(temp.toUtf8());
    } break;
    case AVCOL_TRC_SMPTE428:
        frag.append(GLSL(color.rgb = vec3(52.37 / 48.0) * pow(color.rgb, vec3(2.6));\n));
        break;
    case AVCOL_TRC_ARIB_STD_B67: {
        auto temp = QString("color.rgb *= vec3(%1);\n").arg(MP_REF_WHITE_HLG);
        temp.append(
            QString("color.rgb = mix(vec3(0.5) * sqrt(color.rgb),\n"
                    "                vec3(%1) * log(color.rgb - vec3(%2)) + vec3(%3),\n"
                    "                vec3(lessThan(vec3(1.0), color.rgb)));\n")
                .arg(QString::number(HLG_A), QString::number(HLG_B), QString::number(HLG_C)));
        frag.append(temp.toUtf8());
    } break;
    default: break;
    };
    auto temp = QString("color.rgb *= vec3(1.0/%1);\n").arg(trcNomPeak(colortTrc));
    frag.append(temp.toUtf8());
}

void passDeLinearize(QByteArray &frag, AVColorTransferCharacteristic colortTrc)
{
    if (colortTrc == AVCOL_TRC_LINEAR) {
        return;
    }
    frag.append("\n// pass delinearize\n");
    frag.append(GLSL(color.rgb = clamp(color.rgb, 0.0, 1.0);\n));
    auto temp = QString("color.rgb *= vec3(%1);\n").arg(trcNomPeak(colortTrc));
    frag.append(temp.toUtf8());
    switch (colortTrc) {
    case AVCOL_TRC_BT709:
    case AVCOL_TRC_SMPTE170M:
    case AVCOL_TRC_SMPTE240M:
    case AVCOL_TRC_BT1361_ECG:
    case AVCOL_TRC_BT2020_10:
    case AVCOL_TRC_BT2020_12:
        frag.append(GLSL(color.rgb = pow(color.rgb, vec3(1.0 / 2.4));\n));
        break;
    case AVCOL_TRC_GAMMA22:
        frag.append(GLSL(color.rgb = pow(color.rgb, vec3(1.0 / 2.2));\n));
        break;
    case AVCOL_TRC_GAMMA28:
        frag.append(GLSL(color.rgb = pow(color.rgb, vec3(1.0 / 2.8));\n));
        break;
    case AVCOL_TRC_IEC61966_2_1:
        frag.append("color.rgb = mix(color.rgb * vec3(12.92),\n"
                    "               vec3(1.055) * pow(color.rgb, vec3(1.0/2.4)) - vec3(0.055),\n"
                    "               vec3(lessThanEqual(vec3(0.0031308), color.rgb)));\n");
        break;
    case AVCOL_TRC_SMPTEST2084: {
        auto temp = QString("color.rgb *= vec3(1.0/%1);\n").arg(SMPTEST2048_REF_WHITE / MP_REF_WHITE);
        temp.append(QString("color.rgb = pow(color.rgb, vec3(%1));\n").arg(PQ_M1));
        temp.append(
            QString("color.rgb = (vec3(%1) + vec3(%2) * color.rgb) \n"
                    "             / (vec3(1.0) + vec3(%3) * color.rgb);\n")
                .arg(QString::number(PQ_C1), QString::number(PQ_C2), QString::number(PQ_C3)));
        temp.append(QString("color.rgb = pow(color.rgb, vec3(%1));\n").arg(PQ_M2));
        frag.append(temp.toUtf8());
    } break;
    case AVCOL_TRC_SMPTE428:
        frag.append(GLSL(color.rgb = pow(color.rgb * vec3(48.0 / 52.37), vec3(1.0 / 2.6));\n));
        break;
    case AVCOL_TRC_ARIB_STD_B67: {
        auto temp = QString("color.rgb *= vec3(%1);\n").arg(MP_REF_WHITE_HLG);
        temp.append(
            QString("color.rgb = mix(vec3(0.5) * sqrt(color.rgb),\n"
                    "                vec3(%1) * log(color.rgb - vec3(%2)) + vec3(%3),\n"
                    "                vec3(lessThan(vec3(1.0), color.rgb)));\n")
                .arg(QString::number(HLG_A), QString::number(HLG_B), QString::number(HLG_C)));
        frag.append(temp.toUtf8());
    } break;
    default: break;
    }
}

void finishFragment(QByteArray &frag)
{
    frag.append(GLSL(\n));
    frag.append(GLSL(color.rgb = adjustContrast(color.rgb, contrast);\n));
    frag.append(GLSL(color.rgb = adjustSaturation(color.rgb, saturation);\n));
    frag.append(GLSL(color.rgb = adjustBrightness(color.rgb, brightness);\n));
    frag.append(GLSL(color.rgb = adjustGamma(color.rgb, gamma);\n));
    frag.append(GLSL(color.rgb = adjustHue(color.rgb, hue);\n));
    frag.append(GLSL(FragColor = color;\n));
}

void printShader(const QByteArray &frag)
{
    qInfo().noquote() << "Video fragment shader:\n" << frag;
}

void passGama(QByteArray &frag, AVColorTransferCharacteristic colortTrc)
{
    frag.append("\n// pass gama\n");
    auto temp = QString("color.rgb *= vec3(%1);\n").arg(trcNomPeak(colortTrc));
    frag.append(temp.toUtf8());
}

void passDeGama(QByteArray &frag, AVColorTransferCharacteristic colortTrc)
{
    frag.append("\n// pass de gamma\n");
    float dstMaxLuma = trcNomPeak(colortTrc) * MP_REF_WHITE;
    float dst_range = dstMaxLuma / MP_REF_WHITE;
    if (trcIsHdr(colortTrc)) {
        dst_range = trcNomPeak(colortTrc);
    }
    auto temp = QString("color.rgb *= vec3(%1);\n").arg(1.0 / dst_range);
    frag.append(temp.toUtf8());
}

void passOotf(QByteArray &frag, float peak, AVColorTransferCharacteristic colortTrc)
{
    frag.append("\n// pass ootf\n");
    switch (colortTrc) {
    case AVCOL_TRC_ARIB_STD_B67: {
        float gamma = qMax(1.0, 1.2 + 0.42 * log10(peak * MP_REF_WHITE / 1000.0));
        auto temp = QString("color.rgb *= vec3(%1 * pow(dot(src_luma, color.rgb), %2));\n")
                        .arg(QString::number(peak
                                             / qPow(ARIB_STD_B67_VALUE / MP_REF_WHITE_HLG, gamma)),
                             QString::number(gamma - 1.0));
        frag.append(temp.toUtf8());
    } break;
    default: break;
    }
}

void passInverseOotf(QByteArray &frag, float peak, AVColorTransferCharacteristic colortTrc)
{
    frag.append("\n// pass inverse ootf\n");
    switch (colortTrc) {
    case AVCOL_TRC_ARIB_STD_B67: {
        float gamma = qMax(1.0, 1.2 + 0.42 * log10(peak * MP_REF_WHITE / 1000.0));
        auto temp = QString("color.rgb *= vec3(1.0/%1);\n")
                        .arg(peak / qPow(ARIB_STD_B67_VALUE / MP_REF_WHITE_HLG, gamma));
        temp.append(QString("color.rgb /= vec3(max(1e-6, pow(dot(src_luma, color.rgb), %1)));\n")
                        .arg((gamma - 1.0) / gamma));
        frag.append(temp.toUtf8());
    } break;
    default: break;
    }
}

auto convertPrimaries(QByteArray &header,
                      QByteArray &frag,
                      AVColorPrimaries srcPrimaries,
                      AVColorPrimaries dstPrimaries,
                      QMatrix3x3 &matrix) -> bool
{
    frag.append("\n// convert primaries\n");
    if (srcPrimaries == dstPrimaries) {
        return false;
    }
    if (!ColorUtils::supportConvertColorPrimaries(srcPrimaries)) {
        return false;
    }
    if (!ColorUtils::supportConvertColorPrimaries(dstPrimaries)) {
        return false;
    }

    header.append(GLSL(uniform mat3 cms_matrix;\n));
    frag.append(GLSL(color.rgb = cms_matrix * color.rgb;\n));
    auto csp_src = ColorUtils::getRawPrimaries(srcPrimaries);
    auto csp_dst = ColorUtils::getRawPrimaries(dstPrimaries);
    float m[3][3] = {{0}};
    getCMSMatrix(csp_src, csp_dst, m);
    matrix = QMatrix3x3(&m[0][0]);
    return true;
}

} // namespace Ffmpeg::ShaderUtils
