struct MaterialData
{
    vec3 ambient;
    vec3 diffuse;
    vec4 specular;
};

layout (std430, set=2, binding=0) buffer readonly Material
{
    MaterialData materials[];
} u_Materials;

layout(set=2, binding = 1) uniform sampler2D u_BoxSampler;
layout(set=2, binding = 2) uniform sampler2D u_FaceSampler;
