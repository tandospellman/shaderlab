#version 330 core

out vec4 FragColor;

uniform vec2 u_resolution;

void main()
{
    vec2 uv = gl_FragCoord.xy / u_resolution;

    vec3 color = vec3(
        uv.x,
        uv.y,
        1.0 - uv.x
    );

    FragColor = vec4(color, 1.0);
}


