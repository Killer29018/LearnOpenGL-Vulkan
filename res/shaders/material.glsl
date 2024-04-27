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
