#version 450

layout (location = 0) in vec2 v_UV;

layout (location = 0) out vec4 f_Colour;

layout(binding = 0) uniform sampler2D u_BoxSampler;
layout(binding = 1) uniform sampler2D u_FaceSampler;

void main()
{
    vec4 box = texture(u_BoxSampler, v_UV);
    vec4 face = texture(u_FaceSampler, v_UV);

    f_Colour = mix(box, face, 0.5);
}
