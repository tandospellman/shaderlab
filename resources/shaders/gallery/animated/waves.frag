#version 330 core
out vec4 fragColor;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_frequency;
uniform float u_speed;
void main()
{
    vec2 uv = gl_FragCoord.xy / u_resolution.xy;
    float wave = sin(uv.x * max(u_frequency, 1.0) + u_time * u_speed);
    float line_y = 0.5 + wave * 0.15;
    float line = 1.0 - smoothstep(0.0, 0.012, abs(uv.y - line_y));
    vec3 color = mix(vec3(0.03, 0.04, 0.08), vec3(0.45, 0.40, 1.0), line);
    fragColor = vec4(color, 1.0);
}
