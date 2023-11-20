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
    }

    OpenglShader *q_ptr;

    HdrMetaData srcHdrMetaData;
    HdrMetaData dstHdrMetaData;

    AVColorTransferCharacteristic dstColorTrc;
};

OpenglShader::OpenglShader(QObject *parent)
    : QObject{parent}
    , d_ptr(new OpenglShaderPrivate(this))
{}

OpenglShader::~OpenglShader() = default;

auto OpenglShader::generate(Frame *frame, Tonemap::Type type) -> QByteArray
{
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
    if (type == Tonemap::AUTO && ShaderUtils::trcIsHdr(avFrame->color_trc)) {
        type = Tonemap::ACES_APPROX;
    }
    Tonemap::toneMap(header, frag, type);

    //ShaderUtils::passInverseOotf(frag, d_ptr->dstHdrMetaData.maxLuma, avFrame->color_trc);
    ShaderUtils::passDeGama(frag, d_ptr->dstColorTrc);
    ShaderUtils::passDeLinearize(frag, d_ptr->dstColorTrc);
    ShaderUtils::finishFragment(frag);
    frag.append("\n}\n");
    frag = header + "\n" + frag;
    ShaderUtils::printShader(frag);
    return frag;
}

} // namespace Ffmpeg
