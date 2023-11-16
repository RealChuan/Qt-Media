
vec3 yuv;
vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

yuv.x = texture(tex_y, TexCord).r;
yuv.yz = texture(tex_u, TexCord).gr;

yuv += offset;
color.rgb = yuv * colorConversion;
