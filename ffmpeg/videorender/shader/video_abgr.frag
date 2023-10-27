#version 330 core

in vec2 TexCord;         // 纹理坐标
out vec4 FragColor;      // 输出颜色

uniform sampler2D tex_y;

uniform vec3 offset;
uniform mat3 colorConversion;

void main()
{
    FragColor = texture(tex_y, TexCord).abgr;
}
