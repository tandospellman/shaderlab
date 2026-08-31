#version 330 core
out vec4 fragColor;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_spacing;
void main()
{
    vec2 position = gl_FragCoord.xy - u_resolution * 0.5;
    float radius = length(position);
    float spacing = max(u_spacing, 15.0);
    float ring = mod(radius - u_time * 20.0, spacing);
    float intensity = 1.0 - smoothstep(1.0, 4.0, abs(ring - spacing * 0.5));
    fragColor = vec4(vec3(0.25, 0.60, 1.0) * intensity, 1.0);
}
