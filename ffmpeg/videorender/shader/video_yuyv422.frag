void main()
{
    vec3 yuv;
    vec3 rgb;

    yuv.xyz = texture(tex_y, TexCord).rga;

    yuv += offset;
    rgb = yuv * colorConversion;

    rgb = adjustContrast(rgb, contrast);
    rgb = adjustSaturation(rgb, saturation);
    rgb = adjustBrightness(rgb, brightness);

    FragColor = vec4(rgb, 1.0);
}
