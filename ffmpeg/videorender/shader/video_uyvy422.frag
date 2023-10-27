#version 330 core

in vec2 TexCord;         // 纹理坐标
out vec4 FragColor;      // 输出颜色

uniform sampler2D tex_y;

uniform vec3 offset;
uniform mat3 colorConversion;

void main()
{
    vec3 yuv;
    vec3 rgb;

    int width = textureSize(tex_y, 0).x * 2;
    float tex_x = TexCord.x;
    int pixel = int(floor(width * tex_x)) % 2;
    vec4 tc = texture(tex_y, TexCord).rgba;
    float cb = tc.r;
    float y1 = tc.g;
    float cr = tc.b;
    float y2 = tc.a;
    float y = (pixel == 1) ? y2 : y1;
    yuv = vec3(y, cb, cr);

    yuv += offset;
    rgb = yuv * colorConversion;

    FragColor = vec4(rgb, 1.0);
}
