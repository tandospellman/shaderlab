#version 330 core
out vec4 fragColor;
uniform vec2 u_resolution;
uniform float u_time;
void main()
{
    vec2 uv = gl_FragCoord.xy / u_resolution.xy;
    vec3 color_a = vec3(0.18, 0.25, 0.75);
    vec3 color_b = vec3(0.85, 0.30, 0.55);
    float wave = sin(uv.x * 4.0 + u_time * 0.5);
    vec3 color = mix(color_a, color_b, uv.y + wave * 0.08);
    fragColor = vec4(color, 1.0);
}
