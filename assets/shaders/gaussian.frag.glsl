#version 330 core

uniform vec2      u_resolution;
uniform sampler2D u_tex;
uniform int       u_kernel_size;

in vec2 uv;

out vec4 out_col;

float get_gaussian_coeficient(vec2 offset, float deviation)
{
    return exp(-(offset.x * offset.x + offset.y * offset.y) / (2.0 * deviation * deviation));
}

vec4 get_gaussian_blur(sampler2D tex, vec2 start, vec2 direction, const int kernel_size)
{
    // 1 / (2*PI)
    const float kernel_norm = 0.15915494;

    // the color/weight of each pixel within the kernel
    float total_weight = 0.0;
    vec4  total_color  = vec4(0.0);

    // current offset within the kernel from the center pixel
    vec2 offset = vec2(0.0);

    for (int y = 0; y < kernel_size; ++y)
    {
        offset.y = -0.5 * (float(kernel_size) - 1.0) + float(y);

        for (int x = 0; x < kernel_size; ++x)
        {
            offset.x = -0.5 * (float(kernel_size) - 1.0) + float(x);

            // calculate gaussian weight
            float weight = (kernel_norm / float(kernel_size)) * get_gaussian_coeficient(offset, kernel_size);

            total_weight += weight;
            total_color  += weight * texture2D(u_tex, start + offset * direction);
        }
    }

    // normalize the color
    return total_color / total_weight;
}

void main()
{
    vec2 pixel = 1.0 / u_resolution;

    out_col = get_gaussian_blur(u_tex, gl_FragCoord.xy * pixel, pixel, u_kernel_size);
}
