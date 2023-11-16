
vec3 yuv;
vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

yuv.x = texture(tex_y, TexCord).r;
yuv.y = texture(tex_u, TexCord).r;
yuv.z = texture(tex_v, TexCord).r;

yuv += offset;
color.rgb = yuv * colorConversion;
