#include "openglshader.hpp"
#include "hdrmetadata.hpp"
#include "shaderutils.hpp"

#include <ffmpeg/frame.hpp>
#include <utils/utils.h>

#include <QFile>

extern "C" {
#include <libavutil/frame.h>
}

namespace Ffmpeg {

class OpenglShader::OpenglShaderPrivate
{
public:
    explicit OpenglShaderPrivate(OpenglShader *q)
        : q_ptr(q)
    {}

    void init(Frame *frame)
    {
        auto *avFrame = frame->avFrame();
        srcHdrMetaData = HdrMetaData(frame);
        if (!srcHdrMetaData.maxLuma) {
            srcHdrMetaData.maxLuma = ShaderUtils::trcNomPeak(avFrame->color_trc) * MP_REF_WHITE;
        }
        dstColorTrc = avFrame->color_trc;
        if (dstColorTrc == AVCOL_TRC_LINEAR || ShaderUtils::trcIsHdr(dstColorTrc)) {
            dstColorTrc = AVCOL_TRC_GAMMA22;
        }
        dstHdrMetaData.maxLuma = ShaderUtils::trcNomPeak(dstColorTrc) * MP_REF_WHITE;

        if (dstPrimariesType == ColorUtils::Primaries::AUTO) {
            dstPrimaries = avFrame->color_primaries;
            if (dstPrimaries == AVCOL_PRI_SMPTE170M || dstPrimaries == AVCOL_PRI_BT470BG) {
            } else {
                dstPrimaries = AVCOL_PRI_BT709;
            }
        } else {
            dstPrimaries = ColorUtils::Primaries::getAVColorPrimaries(dstPrimariesType);
        }
    }

    OpenglShader *q_ptr;

    HdrMetaData srcHdrMetaData;
    HdrMetaData dstHdrMetaData;

    AVColorTransferCharacteristic dstColorTrc;
    AVColorPrimaries dstPrimaries;
    ColorUtils::Primaries::Type dstPrimariesType;

    bool isConvertPrimaries = false;
    QMatrix3x3 convertPrimariesMatrix;
};

OpenglShader::OpenglShader(QObject *parent)
    : QObject{parent}
    , d_ptr(new OpenglShaderPrivate(this))
{}

OpenglShader::~OpenglShader() = default;

auto OpenglShader::generate(Frame *frame,
                            Tonemap::Type type,
                            ColorUtils::Primaries::Type destPrimaries) -> QByteArray
{
    d_ptr->dstPrimariesType = destPrimaries;
    d_ptr->init(frame);

    auto *avFrame = frame->avFrame();
    auto format = avFrame->format;

    qInfo() << "Generate Shader:" << format;
    auto header = ShaderUtils::header();
    QByteArray frag("\nvoid main() {\n");
    if (!ShaderUtils::beginFragment(frag, format)) {
        return {};
    }
    ShaderUtils::passLinearize(frag, avFrame->color_trc);
    ShaderUtils::passGama(frag, avFrame->color_trc);
    //ShaderUtils::passOotf(frag, d_ptr->srcHdrMetaData.maxLuma, avFrame->color_trc);

    // Tone map
    if (type == Tonemap ::AUTO) {
        type = ShaderUtils::trcIsHdr(avFrame->color_trc) ? Tonemap::FILMIC : Tonemap::NONE;
    }
    Tonemap::toneMap(header, frag, type);

    // Convert primaries
    d_ptr->isConvertPrimaries = ShaderUtils::convertPrimaries(header,
                                                              frag,
                                                              avFrame->color_primaries,
                                                              d_ptr->dstPrimaries,
                                                              d_ptr->convertPrimariesMatrix);

    //ShaderUtils::passInverseOotf(frag, d_ptr->dstHdrMetaData.maxLuma, avFrame->color_trc);
    ShaderUtils::passDeGama(frag, d_ptr->dstColorTrc);
    ShaderUtils::passDeLinearize(frag, d_ptr->dstColorTrc);
    ShaderUtils::finishFragment(frag);
    frag.append("\n}\n");
    frag = header + "\n" + frag;
    ShaderUtils::printShader(frag);
    return frag;
}

auto OpenglShader::isConvertPrimaries() const -> bool
{
    return d_ptr->isConvertPrimaries;
}

auto OpenglShader::convertPrimariesMatrix() const -> QMatrix3x3
{
    return d_ptr->convertPrimariesMatrix;
}

} // namespace Ffmpeg
