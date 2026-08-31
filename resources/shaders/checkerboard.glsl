#version 330 core

out vec4 FragColor;

uniform vec2 u_resolution;
uniform float scale;

void main()
{
    vec2 uv = gl_FragCoord.xy / u_resolution;

    float safe_scale = max(scale, 1.0);

    vec2 cell = floor(uv * safe_scale);

    float checker =
        mod(cell.x + cell.y, 2.0);

    vec3 dark_color =
        vec3(0.08, 0.08, 0.1);

    vec3 light_color =
        vec3(0.8, 0.8, 0.85);

    vec3 color =
        mix(dark_color, light_color, checker);

    FragColor = vec4(color, 1.0);
}
