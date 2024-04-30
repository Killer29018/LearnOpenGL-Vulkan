#version 450
#extension GL_GOOGLE_include_directive : require

#include "light.glsl"

// layout (location = 0) in flat int v_CurrentLight;
// layout (location = 1) in flat int v_ViewIndex;
// layout (location = 2) in vec4 g_FragPos;
// layout (location = 0) in flat int g_Layer;
// layout (location = 1) in vec4 g_FragPos;

void main()
{
    // ivec3 size = imageSize(u_ShadowMaps);
    //
    // vec3 projected = g_FragPos.xyz / g_FragPos.w;
    // float depth = projected.z;
    //
    // vec2 projectedPosition = projected.xy * 0.5 + 0.5;
    //
    // ivec3 position = ivec3(projectedPosition.xy * size.xy, g_Layer);
    //
    // imageStore(u_ShadowMaps, position, vec4(depth, 0.0, 0.0, 0.0));
}
