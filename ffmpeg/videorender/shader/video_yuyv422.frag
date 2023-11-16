
vec3 yuv;
vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

yuv.xyz = texture(tex_y, TexCord).rga;

yuv += offset;
color.rgb = yuv * colorConversion;
