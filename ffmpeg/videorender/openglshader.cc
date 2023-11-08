#include "openglshader.hpp"

#include <utils/utils.h>

#include <QFile>

extern "C" {
#include <libavutil/pixfmt.h>
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

QByteArray OpenglShader::generate(int format)
{
    qInfo() << "Generate Shader:" << format;
    auto frag = Utils::readAllFile(":/shader/video_header.frag");
    frag.append("\n");
    frag.append(Utils::readAllFile(":/shader/video_color.frag"));
    frag.append("\n");
    switch (format) {
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUV422P:
    case AV_PIX_FMT_YUV444P:
    case AV_PIX_FMT_YUV410P:
    case AV_PIX_FMT_YUV411P: frag.append(Utils::readAllFile(":/shader/video_yuv420p.frag")); break;
    case AV_PIX_FMT_YUYV422: frag.append(Utils::readAllFile(":/shader/video_yuyv422.frag")); break;
    case AV_PIX_FMT_RGB24:
    case AV_PIX_FMT_BGR8:
    case AV_PIX_FMT_RGB8: frag.append(Utils::readAllFile(":/shader/video_rgb24.frag")); break;
    case AV_PIX_FMT_BGR24: frag.append(Utils::readAllFile(":/shader/video_bgr24.frag")); break;
    case AV_PIX_FMT_UYVY422: frag.append(Utils::readAllFile(":/shader/video_uyvy422.frag")); break;
    case AV_PIX_FMT_NV12:
    case AV_PIX_FMT_P010LE: frag.append(Utils::readAllFile(":/shader/video_nv12.frag")); break;
    case AV_PIX_FMT_NV21: frag.append(Utils::readAllFile(":/shader/video_nv21.frag")); break;
    case AV_PIX_FMT_ARGB: frag.append(Utils::readAllFile(":/shader/video_argb.frag")); break;
    case AV_PIX_FMT_RGBA: frag.append(Utils::readAllFile(":/shader/video_rgba.frag")); break;
    case AV_PIX_FMT_ABGR: frag.append(Utils::readAllFile(":/shader/video_abgr.frag")); break;
    case AV_PIX_FMT_BGRA: frag.append(Utils::readAllFile(":/shader/video_bgra.frag")); break;
    default: qWarning() << "UnSupported format:" << format; break;
    }
    frag.append("\n");
    return frag;
}

} // namespace Ffmpeg
