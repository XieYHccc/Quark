#version 450
#extension GL_GOOGLE_include_directive : require
#include "include/uniform_scene.glslh"

layout(location = 1) in vec3 nearPoint; // nearPoint calculated in vertex shader
layout(location = 2) in vec3 farPoint; // farPoint calculated in vertex shader

layout(location = 0) out vec4 outColor;

vec4 Grid(vec3 fragPos3D, float scale) 
{
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);
    vec4 color = vec4(0.3, 0.3, 0.3, 1.0 - min(line, 1.0));

    float line_distantce = 1 / scale;

    // z axis
    if(fragPos3D.x > -line_distantce * minimumx && fragPos3D.x < line_distantce * minimumx)
        color.z = 1.0;
    // x axis
    if(fragPos3D.z > -line_distantce * minimumz && fragPos3D.z < line_distantce * minimumz)
        color.x = 1.0;
    
    return color;
}

float ComputeDepth(vec3 pos) 
{
    vec4 clip_space_pos = sceneData.viewproj * vec4(pos.xyz, 1.0);
    return (clip_space_pos.z / clip_space_pos.w);
}

float ComputeLinearDepth(float fragDepth, float near, float far) 
{
    float linear_depth = near * far / (far - fragDepth * (far - near)); // get linear value between 0.01 and 100
    return (linear_depth  - near) / (far - near); // normalize
}

void main() 
{
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);
    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);

    float frag_depth = ComputeDepth(fragPos3D);
    float linear_depth = ComputeLinearDepth(frag_depth, 0.1f, 100.f);
    float fading = max(0, (1.f - 4 * linear_depth));

    gl_FragDepth = frag_depth;

    outColor = Grid(fragPos3D, 1) * float(t > 0);
    outColor.a *= fading;
}