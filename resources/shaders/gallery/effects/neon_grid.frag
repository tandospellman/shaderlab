#version 330 core
out vec4 fragColor;
uniform vec2 u_resolution;
uniform float u_time;
uniform vec3 u_color;
void main()
{
    vec2 uv = (gl_FragCoord.xy - u_resolution * 0.5) / u_resolution.y;
    uv.y += u_time * 0.08;
    vec2 grid = abs(fract(uv * 8.0) - 0.5);
    float line = min(grid.x, grid.y);
    float glow = 0.012 / max(line, 0.003);
    vec3 background = vec3(0.01, 0.015, 0.035);
    fragColor = vec4(background + u_color * glow * 0.04, 1.0);
}
