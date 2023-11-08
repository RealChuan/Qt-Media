void main()
{
    vec4 color = texture(tex_y, TexCord).gbar;

    color.rgb = adjustContrast(color.rgb, contrast);
    color.rgb = adjustSaturation(color.rgb, saturation);
    color.rgb = adjustBrightness(color.rgb, brightness);

    FragColor = color;
}
