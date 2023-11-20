#include "tonemap.hpp"
#include "shaderutils.hpp"

#include <utils/utils.h>

namespace Ffmpeg {

Tonemap::Tonemap(QObject *parent)
    : QObject(parent)
{}

// Kodi
// https://github.com/xbmc/xbmc/blob/1e499e091f7950c70366d64ab2d8c4f3a18cfbfa/system/shaders/GL/1.5/gl_tonemap.glsl#L4
// MPV
// static void pass_tone_map(struct gl_shader_cache *sc,
//                           float src_peak,
//                           float dst_peak,
//                           const struct gl_tone_map_opts *opts)

// https://github.com/64/64.github.io/blob/src/code/tonemapping/tonemap.cpp#L40

void Tonemap::toneMap(QByteArray &header, QByteArray &frag, Type type)
{
    frag.append("\n// pass tone map\n");
    switch (type) {
    case REINHARD: frag.append(GLSL(color.rgb = reinhard(color.rgb);\n)); break;
    case REINHARD_JODIE: frag.append(GLSL(color.rgb = reinhard_jodie(color.rgb);\n)); break;
    case CONST_LUMINANCE_REINHARD:
        frag.append(GLSL(color.rgb = const_luminance_reinhard(color.rgb);\n));
        break;
    case HABLE: frag.append(GLSL(color.rgb = hable(color.rgb);\n)); break;
    case ACES: frag.append(GLSL(color.rgb = aces(color.rgb);\n)); break;
    case ACES_FITTED: frag.append(GLSL(color.rgb = aces_fitted(color.rgb);\n)); break;
    case ACES_APPROX: frag.append(GLSL(color.rgb = aces_approx(color.rgb);\n)); break;
    case FILMIC: frag.append(GLSL(color.rgb = filmic(color.rgb);\n)); break;
    case UNCHARTED2_FILMIC: frag.append(GLSL(color.rgb = uncharted2_filmic(color.rgb);\n)); break;
    default: return;
    }
    header.append(Utils::readAllFile(":/shader/tonemap.frag"));
}

} // namespace Ffmpeg
