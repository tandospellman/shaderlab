#version 330 core
out vec4 fragColor;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_speed;
void main()
{
    vec2 uv = gl_FragCoord.xy / u_resolution.xy;
    float scanner_x = fract(u_time * 0.15 * max(u_speed, 0.1));
    float scanner = 1.0 - smoothstep(0.0, 0.035, abs(uv.x - scanner_x));
    vec3 background = vec3(0.02, 0.04, 0.07);
    vec3 scanner_color = vec3(0.15, 0.90, 0.70);
    fragColor = vec4(background + scanner_color * scanner, 1.0);
}
