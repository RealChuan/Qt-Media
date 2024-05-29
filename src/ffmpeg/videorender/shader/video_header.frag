#version 330 core

in vec2 TexCord;    // 纹理坐标
out vec4 FragColor; // 输出颜色

uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;

uniform vec3 offset;
uniform mat3 colorConversion;
