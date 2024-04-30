#version 450
#extension GL_GOOGLE_include_directive : require

#include "light.glsl"

layout (location = 0) in flat int v_CurrentLight;
layout (location = 1) in flat int v_ViewIndex;

void main()
{
    ivec3 position = ivec3(gl_FragCoord.xy, v_CurrentLight * 6 + v_ViewIndex);
    float v = imageLoad(u_ShadowMaps, position).r;

    if (gl_FragCoord.z < v)
        imageStore(u_ShadowMaps, position, vec4(gl_FragCoord.z, 0.0, 0.0, 0.0));
}