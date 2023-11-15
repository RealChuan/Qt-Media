void main()
{
    vec3 yuv;
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

    yuv.x = texture(tex_y, TexCord).r;
    yuv.yz = texture(tex_u, TexCord).rg;

    yuv += offset;
    color.rgb = yuv * colorConversion;

    color.rgb = adjustContrast(color.rgb, contrast);
    color.rgb = adjustSaturation(color.rgb, saturation);
    color.rgb = adjustBrightness(color.rgb, brightness);

    FragColor = color;
}
