struct LightData
{
    vec3 position;
    vec4 colour;
    mat4 model;
};

layout (std430, binding=3) buffer readonly Lights
{
    int lightCount;
    vec4 ambient;
    LightData lights[];
} u_Lights;
