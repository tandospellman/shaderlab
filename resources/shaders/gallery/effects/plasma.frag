#version 330 core
out vec4 fragColor;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_speed;
void main()
{
    vec2 uv = (gl_FragCoord.xy - u_resolution * 0.5) / u_resolution.y;
    float time = u_time * max(u_speed, 0.1);
    float value = sin(uv.x * 8.0 + time);
    value += sin(uv.y * 10.0 - time * 1.2);
    value += sin(length(uv) * 12.0 - time * 2.0);
    value /= 3.0;
    vec3 color = 0.5 + 0.5 * cos(value + vec3(0.0, 2.0, 4.0));
    fragColor = vec4(color, 1.0);
}
