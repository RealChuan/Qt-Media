#version 330 core
in vec2 TexCord;    // 纹理坐标
out vec4 FragColor; // 输出颜色

uniform sampler2D tex;

void main()
{
    FragColor = texture(tex, TexCord);
}
