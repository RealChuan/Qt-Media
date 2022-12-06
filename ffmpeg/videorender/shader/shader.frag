#version 330 core
in vec2 TexCord;         // 纹理坐标
uniform int format = -1; // 像素格式
uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;
uniform sampler2D tex_uv;
uniform sampler2D tex_rgba;

//******************************code form SDL***********************
//#define JPEG_SHADER_CONSTANTS
// YUV offset \n"
// const vec3 offset = vec3(0, -0.501960814, -0.501960814);

// // RGB coefficients \n"
// const vec3 Rcoeff = vec3(1, 0.000, 1.402);
// const vec3 Gcoeff = vec3(1, -0.3441, -0.7141);
// const vec3 Bcoeff = vec3(1, 1.772, 0.000);

//#define BT601_SHADER_CONSTANTS
// YUV offset \n"
// const vec3 offset = vec3(-0.0627451017, -0.501960814, -0.501960814);

// // RGB coefficients \n"
// const vec3 Rcoeff = vec3(1.1644, 0.000, 1.596);
// const vec3 Gcoeff = vec3(1.1644, -0.3918, -0.813);
// const vec3 Bcoeff = vec3(1.1644, 2.0172, 0.000);

//#define BT709_SHADER_CONSTANTS
// YUV offset \n"
const vec3 offset = vec3(-0.0627451017, -0.501960814, -0.501960814);

// RGB coefficients \n"
const vec3 Rcoeff = vec3(1.1644, 0.000, 1.7927);
const vec3 Gcoeff = vec3(1.1644, -0.2132, -0.5329);
const vec3 Bcoeff = vec3(1.1644, 2.1124, 0.000);
//**************************************************************

void main()
{
    vec3 yuv;
    vec3 rgb;

    if (format == 0 || format == 4 || format == 5 || format == 6 || format == 7) { // YUV420P
        yuv.x = texture2D(tex_y, TexCord).r;
        yuv.y = texture2D(tex_u, TexCord).r;
        yuv.z = texture2D(tex_v, TexCord).r;
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
    } else if (format == 23) { // NV12
        yuv.x = texture2D(tex_y, TexCord.st).r;
        yuv.y = texture2D(tex_uv, TexCord.st).r;
        yuv.z = texture2D(tex_uv, TexCord.st).a;
    } else if (format == 24) { // NV21
        yuv.x = texture2D(tex_y, TexCord.st).r;
        yuv.y = texture2D(tex_uv, TexCord.st).a;
        yuv.z = texture2D(tex_uv, TexCord.st).r;
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
    rgb.r = dot(yuv, Rcoeff);
    rgb.g = dot(yuv, Gcoeff);
    rgb.b = dot(yuv, Bcoeff);

    gl_FragColor = vec4(rgb, 1.0);
}
