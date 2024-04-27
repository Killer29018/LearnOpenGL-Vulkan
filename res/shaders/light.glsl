struct LightData
{
    vec3 position;
    mat4 model;
    vec3 diffuse;
    vec3 specular;
};

layout (std430, set=0, binding=0) buffer readonly Lights
{
    int lightCount;
    vec4 ambient;
    LightData lights[];
} u_Lights;
