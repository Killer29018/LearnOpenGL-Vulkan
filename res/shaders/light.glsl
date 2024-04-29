struct LightData
{
    vec3 position;
    mat4 model;
    mat4 viewProj;
    vec3 diffuse;
    vec3 specular;
    vec3 attenuation;
};

layout (std430, set=0, binding=0) buffer readonly Lights
{
    int lightCount;
    vec4 ambient;
    LightData lights[];
} u_Lights;

layout (r32f, set=0, binding=1) uniform image2DArray u_ShadowMaps;
