#include "tonemap.hpp"
#include "shaderutils.hpp"

#include <utils/utils.h>

namespace Ffmpeg {

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
    case CLIP: frag.append(GLSL(color.rgb = clip(color.rgb);\n)); break;
    case LINEAR: frag.append(GLSL(color.rgb = linear(color.rgb);\n)); break;
    case GAMMA: frag.append(GLSL(color.rgb = gamma(color.rgb);\n)); break;
    case REINHARD: frag.append(GLSL(color.rgb = reinhard(color.rgb);\n)); break;
    case HABLE: frag.append(GLSL(color.rgb = hable(color.rgb);\n)); break;
    case MOBIUS: frag.append(GLSL(color.rgb = mobius(color.rgb);\n)); break;
    case ACES: frag.append(GLSL(color.rgb = aces(color.rgb);\n)); break;
    case FILMIC: frag.append(GLSL(color.rgb = filmic(color.rgb);\n)); break;
    default: return;
    }
    header.append(Utils::readAllFile(":/shader/tonemap.frag"));
}

} // namespace Ffmpeg
