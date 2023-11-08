void main()
{
    vec3 yuv;
    vec3 rgb;

    yuv.x = texture(tex_y, TexCord).r;
    yuv.y = texture(tex_u, TexCord).r;
    yuv.z = texture(tex_v, TexCord).r;

    yuv += offset;
    rgb = yuv * colorConversion;

    rgb = adjustContrast(rgb, contrast);
    rgb = adjustSaturation(rgb, saturation);
    rgb = adjustBrightness(rgb, brightness);

    FragColor = vec4(rgb, 1.0);
}
