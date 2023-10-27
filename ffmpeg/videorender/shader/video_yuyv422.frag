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

    yuv.xyz = texture(tex_y, TexCord).rga;

    yuv += offset;
    rgb = yuv * colorConversion;

    FragColor = vec4(rgb, 1.0);
}
