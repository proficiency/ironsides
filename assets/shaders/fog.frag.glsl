#version 330 core

in vec2 uv;

out vec4 out_col;

uniform float time;
uniform float mask_radius;
uniform vec2  mask_center;
uniform vec2  resolution;

float fade(float t)
{
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}
vec2 smth(vec2 x)
{
    return vec2(fade(x.x), fade(x.y));
}

vec2 hash(vec2 co)
{
    float m = dot(co, vec2(12.9898, 78.233));
    return fract(vec2(sin(m), cos(m)) * 43758.5453) * 2. - 1.;
}

float perlin(vec2 uv)
{
    vec2 PT   = floor(uv);
    vec2 pt   = fract(uv);
    vec2 mmpt = smth(pt);

    vec4 grads = vec4(
        dot(hash(PT + vec2(.0, 1.)), pt - vec2(.0, 1.)),
        dot(hash(PT + vec2(1., 1.)), pt - vec2(1., 1.)),
        dot(hash(PT + vec2(.0, .0)), pt - vec2(.0, .0)),
        dot(hash(PT + vec2(1., .0)), pt - vec2(1., 0.))
    );

    return 5. * mix(mix(grads.z, grads.w, mmpt.x), mix(grads.x, grads.y, mmpt.x), mmpt.y);
}

float fbm(vec2 uv)
{
    float noise = 0;

    noise += .50000 * perlin(2. * uv);
    noise += .25000 * perlin(4. * uv);
    noise += .12500 * perlin(8. * uv);
    noise += .06250 * perlin(16. * uv);
    noise += .03125 * perlin(32. * uv);

    return noise;
}

void main()
{
    float aspect_ratio = resolution.x / resolution.y;

    float dx                   = (uv.x - mask_center.x) * aspect_ratio;
    float dy                   = uv.y - mask_center.y;
    float distance_from_center = sqrt(dx * dx + dy * dy);

    float scaled_mask_radius = 0.5 * min(resolution.x, resolution.y);

    float mask = smoothstep(mask_radius - 0.01, mask_radius, distance_from_center);

    float n   = fbm(uv * 0.5 + time * 0.1) * 0.7;
    float n2  = fbm(uv * 0.5 - time * 0.1) * 0.7;
    n        -= n2;

    n *= mask;

    out_col = vec4(min(n, 0.3));
}
