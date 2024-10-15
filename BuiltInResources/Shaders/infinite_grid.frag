#version 450
#extension GL_GOOGLE_include_directive : require
#include "include/uniform_scene.glslh"

layout(location = 1) in vec3 nearPoint; // nearPoint calculated in vertex shader
layout(location = 2) in vec3 farPoint; // farPoint calculated in vertex shader

layout(location = 0) out vec4 outColor;

// scale better be a multiple of 10 which make more sense
// scale = 1 : unit is meter, scale = 10, unit = centimeter .....
vec4 Grid(vec3 fragPos3D, float scale, float lineWidth) 
{
    // scale coordinates to control grid density
    vec2 coord = fragPos3D.xz * scale;

    // compute screen-space derivatives for anti-aliasing
    vec2 derivative = fwidth(coord);

    //calculate distance from the nearest grid line
    vec2 distance = abs(fract(coord - 0.5) - 0.5) / derivative;

    // determine the intensity of the grid lines
    float line = min(distance.x, distance.y) / lineWidth;
    float alpha = 1.0 - clamp(line, 0.0, 1.0);

    // base grid color
    vec4 color = vec4(0.2, 0.2, 0.2, alpha);

    // highlight the Z axis (X = 0)
    if (abs(fragPos3D.x) < (0.1 / scale))
        color.z = 1.0;

    // highlight the X axis (Z = 0)
    if (abs(fragPos3D.z) < (0.1 / scale))
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
    float z = fragDepth * 2.0 - 1.0; // Back to NDC
    float linear_depth = (2.0 * near * far) / (far + near - z * (far - near));
    return (linear_depth - near) / (far - near); // normalize to [0,1]
}

void main() 
{
    // calculate the intersection parameter 't' with the Y=0 plane
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);

    if (t < 0.0 || t > 1.0)
        discard;

    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);

    float frag_depth = ComputeDepth(fragPos3D);

    // transform fragment position to eye space
    vec4 eyePos4D = sceneData.view * vec4(fragPos3D, 1.0);
    float eyeDepth = -eyePos4D.z; // Distance along the view direction

    // define fade start and end distances
    const float startFadeDist = 0.0;  // Start fading at 10 units
    const float endFadeDist = 200.0;    // Fully faded at 50 units

    // compute the fading factor using smoothstep for gradual fading
    float fading = 1.0 - smoothstep(startFadeDist, endFadeDist, eyeDepth);

    gl_FragDepth = frag_depth;

    outColor = Grid(fragPos3D, 0.1, 3) * float(t > 0);
    outColor.a *= fading;
}