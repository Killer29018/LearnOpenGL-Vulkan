#version 450
#extension GL_GOOGLE_include_directive : require

#include "light.glsl"
// #include "object.glsl"
#include "material.glsl"

layout (location = 0) in vec2 v_UV;
layout (location = 1) in vec4 v_Colour;
layout (location = 2) in vec3 v_Normal;
layout (location = 3) in vec4 v_FragPos;
layout (location = 4) in flat int v_MaterialIndex;

layout (location = 0) out vec4 f_Position;
layout (location = 1) out vec4 f_Normal;
layout (location = 2) out vec4 f_TexData;

void main()
{
    f_Position = vec4(v_FragPos.xyzw);
    f_Normal = vec4(v_Normal, 0.0);
    f_TexData = vec4(v_UV.xy, v_MaterialIndex, 1.0);
}
