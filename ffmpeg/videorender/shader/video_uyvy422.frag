
vec3 yuv;
vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

int width = textureSize(tex_y, 0).x * 2;
float tex_x = TexCord.x;
int pixel = int(floor(width * tex_x)) % 2;
vec4 tc = texture(tex_y, TexCord).rgba;
float cb = tc.r;
float y1 = tc.g;
float cr = tc.b;
float y2 = tc.a;
float y = (pixel == 1) ? y2 : y1;
yuv = vec3(y, cb, cr);

yuv += offset;
color.rgb = yuv * colorConversion;
