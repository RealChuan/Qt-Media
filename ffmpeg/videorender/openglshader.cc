#include "openglshader.hpp"
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

    OpenglShader *q_ptr;
};

OpenglShader::OpenglShader(QObject *parent)
    : QObject{parent}
    , d_ptr(new OpenglShaderPrivate(this))
{}

OpenglShader::~OpenglShader() = default;

auto OpenglShader::generate(Frame *frame) -> QByteArray
{
    auto *avFrame = frame->avFrame();
    auto format = avFrame->format;
    qInfo() << "Generate Shader:" << format;
    auto frag = ShaderUtils::header();
    if (!ShaderUtils::beginFragment(frag, format)) {
        return {};
    }
    ShaderUtils::passLinearize(frag, avFrame->color_trc);

    auto temp = QString("color.rgb *= vec3(%1);\n").arg(ShaderUtils::trcNomPeak(avFrame->color_trc));
    frag.append(temp.toUtf8());

    // HDR
    // ShaderUtils::toneMap(frag, avFrame);

    auto color_trc = ShaderUtils::trcIsHdr(avFrame->color_trc) ? AVCOL_TRC_GAMMA22
                                                               : avFrame->color_trc;
    ShaderUtils::passDeLinearize(frag, color_trc);
    ShaderUtils::finishFragment(frag);
    ShaderUtils::printShader(frag);
    return frag;
}

} // namespace Ffmpeg
