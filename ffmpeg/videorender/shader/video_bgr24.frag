void main()
{
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    color.rgb = texture(tex_y, TexCord).bgr;

    color.rgb = adjustContrast(color.rgb, contrast);
    color.rgb = adjustSaturation(color.rgb, saturation);
    color.rgb = adjustBrightness(color.rgb, brightness);

    FragColor = color;
}
