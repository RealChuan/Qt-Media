void main()
{
    FragColor = texture(tex_y, TexCord).rgba;

    color.rgb = adjustContrast(color.rgb, contrast);
    color.rgb = adjustSaturation(color.rgb, saturation);
    color.rgb = adjustBrightness(color.rgb, brightness);

    FragColor = color;
}
