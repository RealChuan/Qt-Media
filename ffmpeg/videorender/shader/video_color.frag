uniform float contrast;
uniform float saturation;
uniform float brightness;

vec3 adjustBrightness(vec3 rgb, float brightness) // 调整亮度
{
    brightness = clamp(brightness, -1.0, 1.0);
    return clamp(rgb + brightness, 0.0, 1.0);
}

// 调整对比度 和使用sws_setColorspaceDetails效果不太一样
vec3 adjustContrast(vec3 rgb, float contrast)
{
    contrast = clamp(contrast, 0.0, 2.0);
    vec3 contrastColor = (rgb - vec3(0.5)) * contrast + vec3(0.5);
    return clamp(contrastColor, vec3(0.0), vec3(1.0));
}

vec3 rgb2Hsv(vec3 rgb)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(rgb.bg, K.wz), vec4(rgb.gb, K.xy), step(rgb.b, rgb.g));
    vec4 q = mix(vec4(p.xyw, rgb.r), vec4(rgb.r, p.yzx), step(p.x, rgb.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2Rgb(vec3 hsv)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(hsv.xxx + K.xyz) * 6.0 - K.www);
    return hsv.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), hsv.y);
}

vec3 adjustSaturation(vec3 rgb, float saturation) // 调整饱和度
{
    saturation = clamp(saturation, 0.0, 2.0);
    vec3 hsv = rgb2Hsv(rgb);
    hsv.y = hsv.y * saturation;
    return hsv2Rgb(hsv);
}