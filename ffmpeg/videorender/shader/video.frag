#version 330 core
in vec2 TexCord;         // 纹理坐标

uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;
uniform sampler2D tex_uv;
uniform sampler2D tex_rgba;

uniform int format = -1; // 像素格式
uniform vec3 offset;
uniform mat3 colorConversion;

void main()
{
    vec3 yuv;
    vec3 rgb;

    if (format == 0 || format == 4 || format == 5 || format == 6 || format == 7) { // YUV420P
        yuv.x = texture(tex_y, TexCord).r;
        yuv.y = texture(tex_u, TexCord).r;
        yuv.z = texture(tex_v, TexCord).r;
    } else if (format == 1) { // YUYV422
        yuv.x = texture(tex_rgba, TexCord).r;
        yuv.y = texture(tex_rgba, TexCord).g;
        yuv.z = texture(tex_rgba, TexCord).a;
    } else if (format == 2 || format == 17 || format == 20) { // RGB24
        gl_FragColor = vec4(texture(tex_rgba, TexCord).rgb, 1);
        return;
    } else if (format == 3) { // BGR24
        gl_FragColor = vec4(texture(tex_rgba, TexCord).bgr, 1);
        return;
    } else if (format == 15) { // UYVY422
        int width = textureSize(tex_rgba, 0).x * 2;
        float tex_x = TexCord.x;
        int pixel = int(floor(width * tex_x)) % 2;
        vec4 tc = texture(tex_rgba, TexCord).rgba;
        float cb = tc.r;
        float y1 = tc.g;
        float cr = tc.b;
        float y2 = tc.a;
        float y = (pixel == 1) ? y2 : y1;
        yuv = vec3(y, cb, cr);
    } else if (format == 23 || format == 161) { // NV12
        yuv.x = texture(tex_y, TexCord).r;
        yuv.y = texture(tex_uv, TexCord).r;
        yuv.z = texture(tex_uv, TexCord).a;
    } else if (format == 24) { // NV21
        yuv.x = texture(tex_y, TexCord).r;
        yuv.y = texture(tex_uv, TexCord).a;
        yuv.z = texture(tex_uv, TexCord).r;
    } else if (format == 25) { // ARGB
        gl_FragColor = texture(tex_rgba, TexCord).gbar;
        return;
    } else if (format == 26) { // RGBA
        gl_FragColor = texture(tex_rgba, TexCord);
        return;
    } else if (format == 27) { // ABGR
        gl_FragColor = texture(tex_rgba, TexCord).abgr;
        return;
    } else if (format == 28) { // BGRA
        gl_FragColor = texture(tex_rgba, TexCord).bgra;
        return;
    } else if (format == 64) { // YUV420P10LE
        vec3 yuv_l;
        vec3 yuv_h;
        yuv_l.x = texture(tex_y, TexCord).r;
        yuv_h.x = texture(tex_y, TexCord).a;
        yuv_l.y = texture(tex_u, TexCord).r;
        yuv_h.y = texture(tex_u, TexCord).a;
        yuv_l.z = texture(tex_v, TexCord).r;
        yuv_h.z = texture(tex_v, TexCord).a;
        yuv = (yuv_l * 255.0 + yuv_h * 255.0 * 256.0) / (1023.0);
    } else if (format == 120) { // 0RGB
        gl_FragColor = vec4(texture(tex_rgba, TexCord).gba, 1);
        return;
    } else if (format == 121) { // RGB0
        gl_FragColor = vec4(texture(tex_rgba, TexCord).rgb, 1);
        return;
    } else if (format == 122) { // 0BGR
        gl_FragColor = vec4(texture(tex_rgba, TexCord).abg, 1);
        return;
    } else if (format == 123) { // BGR0
        gl_FragColor = vec4(texture(tex_rgba, TexCord).bgr, 1);
        return;
    } else {
    }

    yuv += offset;
    rgb = yuv * colorConversion;

    gl_FragColor = vec4(rgb, 1.0);
}