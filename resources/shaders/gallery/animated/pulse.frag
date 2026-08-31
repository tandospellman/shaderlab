#version 330 core
out vec4 fragColor;
uniform vec2 u_resolution;
uniform float u_time;
uniform vec3 u_color;
void main()
{
    vec2 uv = (gl_FragCoord.xy - u_resolution * 0.5) / u_resolution.y;
    float radius = 0.25 + sin(u_time * 2.0) * 0.05;
    float circle = 1.0 - smoothstep(radius, radius + 0.015, length(uv));
    fragColor = vec4(u_color * circle, 1.0);
}
