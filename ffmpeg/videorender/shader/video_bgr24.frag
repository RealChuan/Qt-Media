void main()
{
    vec3 rgb = texture(tex_y, TexCord).bgr;

    rgb = adjustContrast(rgb, contrast);
    rgb = adjustSaturation(rgb, saturation);
    rgb = adjustBrightness(rgb, brightness);

    FragColor = vec4(rgb, 1.0);
}
