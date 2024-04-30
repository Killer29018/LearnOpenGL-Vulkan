struct LightData
{
    vec3 position;
    mat4 model;
    vec3 diffuse;
    vec3 specular;
    vec3 attenuation;
    mat4 proj;
    mat4 view[6];
};

layout (std430, set=0, binding=0) buffer readonly Lights
{
    int lightCount;
    vec4 ambient;
    LightData lights[];
} u_Lights;

layout (set=0, binding=1) uniform sampler2DArray u_ShadowMaps;
