#version 330 core
out vec4 fragColor;
uniform vec2 u_resolution;
uniform float u_scale;
void main()
{
    vec2 uv = gl_FragCoord.xy / u_resolution.xy;
    float scale = max(u_scale, 1.0);
    vec2 grid = floor(uv * scale);
    float pattern = mod(grid.x + grid.y, 2.0);
    vec3 color = mix(vec3(0.08), vec3(0.75, 0.80, 0.95), pattern);
    fragColor = vec4(color, 1.0);
}
