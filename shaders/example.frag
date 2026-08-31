#version 330 core

in vec2 v_uv;

out vec4 out_color;

uniform float u_time;
uniform vec2 u_resolution;

uniform float brightness;
uniform float speed;
uniform vec2 center;
uniform vec3 rgb;
uniform vec4 bias;

uniform float weights[4];

void main()
{
    vec2 uv = v_uv;

    float d = distance(uv, center);

    float weight_sum =
        weights[0] +
        weights[1] +
        weights[2] +
        weights[3];

    vec3 wave =
        0.5 + 0.5 * cos(
            u_time * speed +
            d * 10.0 * weight_sum +
            vec3(0.0, 2.0, 4.0)
        );

    vec3 color =
        wave * rgb * brightness + bias.rgb;

    out_color = vec4(color, bias.a);
}
//test2
//test 3
