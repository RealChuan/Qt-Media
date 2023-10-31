#version 330 core

in vec2 TexCord;    // 纹理坐标
out vec4 FragColor; // 输出颜色

uniform sampler2D tex_y;
uniform sampler2D tex_u;

uniform vec3 offset;
uniform mat3 colorConversion;

uniform float contrast;
uniform float saturation;
uniform float brightness;

vec3 adjustContrast(vec3 rgb, float contrast) // 调整对比度
{
    return clamp((rgb - vec3(0.5)) * contrast + vec3(0.5), vec3(0.0), vec3(1.0));
}

vec3 adjustSaturation(vec3 rgb, float saturation) // 调整饱和度
{
    return mix(vec3(dot(rgb, vec3(0.2126, 0.7152, 0.0722))), rgb, 1.0 + saturation);
}

vec3 adjustBrightness(vec3 rgb, float brightness) // 调整亮度
{
    return clamp(rgb + vec3(brightness), vec3(0.0), vec3(1.0));
}

void main()
{
    vec3 yuv;
    vec3 rgb;

    yuv.x = texture(tex_y, TexCord).r;
    yuv.yz = texture(tex_u, TexCord).rg;

    yuv += offset;
    rgb = yuv * colorConversion;

    rgb = adjustContrast(rgb, contrast);
    rgb = adjustSaturation(rgb, saturation);
    rgb = adjustBrightness(rgb, brightness);

    FragColor = vec4(rgb, 1.0);
}
