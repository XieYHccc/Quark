#version 450

layout(set = 2, binding = 0) uniform sampler2D uSkybox;
layout(location = 0) in vec3 vDirection;
layout(location = 0) out vec4 outFragColor;

void main()
{
    vec3 v = normalize(vDirection);
    if (abs(v.x) < 0.00001)
        v.x = 0.00001;

    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= vec2(0.1591, 0.3183);
    uv += 0.5;

    vec3 color = textureLod(uSkybox, uv, 0.0).rgb;
    outFragColor = vec4(color, 1.0);
}