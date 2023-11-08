void main()
{
    vec3 yuv;
    vec3 rgb;

    yuv.x = texture(tex_y, TexCord).r;
    yuv.yz = texture(tex_u, TexCord).gr;

    yuv += offset;
    rgb = yuv * colorConversion;

    rgb = adjustContrast(rgb, contrast);
    rgb = adjustSaturation(rgb, saturation);
    rgb = adjustBrightness(rgb, brightness);

    FragColor = vec4(rgb, 1.0);
}
