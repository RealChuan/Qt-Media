
vec3 clip(vec3 color)
{
    return clamp(color, vec3(0.0), vec3(1.0));
}

vec3 linear(vec3 color)
{
    return color;
}

vec3 gamma(vec3 color)
{
    return pow(color, vec3(2.2));
}

vec3 reinhard(vec3 color)
{
    return color / (color + vec3(1.0));
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

vec3 mobius(vec3 color)
{
    color = max(vec3(0.0), color - vec3(0.004));
    color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
    return pow(color, vec3(2.2));
}

vec3 aces(vec3 color)
{
    color = color * (color + 0.0245786) / (color * (0.983729 * color + 0.4329510) + 0.238081);
    return pow(color, vec3(1.0 / 2.2));
}

vec3 filmic(vec3 color)
{
    color = max(vec3(0.0), color - vec3(0.004));
    color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
    return pow(color, vec3(2.2));
}
