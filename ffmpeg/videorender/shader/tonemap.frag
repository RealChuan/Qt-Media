
float luminance(vec3 color)
{
    return dot(color, vec3(0.2126f, 0.7152f, 0.0722f));
}

float lerp(float a, float b, float t)
{
    return a * (1.0f - t) + b * t;
}

vec3 lerp(vec3 a, vec3 b, vec3 t)
{
    return vec3(lerp(a.x, b.x, t.x), lerp(a.y, b.y, t.y), lerp(a.z, b.z, t.z));
}

vec3 mul(const mat3 m, const vec3 v)
{
    vec3 result;
    result.x = dot(m[0], v);
    result.y = dot(m[1], v);
    result.z = dot(m[2], v);
    return result;
}

vec3 rtt_and_odt_fit(vec3 v)
{
    vec3 a = v * (v + vec3(0.0245786)) - vec3(0.000090537);
    vec3 b = v * (vec3(0.983729) * v + vec3(0.4329510)) + vec3(0.238081);
    return a / b;
}

vec3 reinhard(vec3 color)
{
    return color / (color + vec3(1.0));
}

vec3 reinhard_jodie(vec3 v)
{
    float l = luminance(v);
    vec3 tv = v / (1.0f + v);
    return lerp(v / (1.0f + l), tv, tv);
}

vec3 const_luminance_reinhard(vec3 c)
{
    vec3 lv = vec3(0.2126f, 0.7152f, 0.0722f);
    vec3 nv = lv / (1.0f - lv);
    c /= 1.0f + dot(c, vec3(lv));
    vec3 nc = vec3(max(c.x - 1.0f, 0.0f), max(c.y - 1.0f, 0.0f), max(c.z - 1.0f, 0.0f)) * nv;
    return c + vec3(nc.y + nc.z, nc.x + nc.z, nc.x + nc.y);
}

vec3 hable(vec3 color)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}

vec3 aces(vec3 color)
{
    color = color * (color + 0.0245786) / (color * (0.983729 * color + 0.4329510) + 0.238081);
    return pow(color, vec3(1.0 / 2.2));
}

vec3 aces_fitted(vec3 v)
{
    const mat3 aces_input_matrix = mat3(vec3(0.59719, 0.35458, 0.04823),
                                        vec3(0.07600, 0.90834, 0.01566),
                                        vec3(0.02840, 0.13383, 0.83777));

    const mat3 aces_output_matrix = mat3(vec3(1.60475, -0.53108, -0.07367),
                                         vec3(-0.10208, 1.10813, -0.00605),
                                         vec3(-0.00327, -0.07276, 1.07602));
    v = mul(aces_input_matrix, v);
    v = rtt_and_odt_fit(v);
    return mul(aces_output_matrix, v);
}

vec3 aces_approx(vec3 v)
{
    v *= 0.6;
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((v * (a * v + b)) / (v * (c * v + d) + e), 0.0, 1.0);
}

vec3 filmic(vec3 color)
{
    color = max(vec3(0.0), color - vec3(0.004));
    color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
    return pow(color, vec3(2.2));
}

vec3 uncharted2_filmic(vec3 v)
{
    float exposure_bias = 2.0f;
    vec3 curr = hable(v * exposure_bias);

    vec3 W = vec3(11.2f);
    vec3 white_scale = vec3(1.0f) / hable(W);
    return curr * white_scale;
}
