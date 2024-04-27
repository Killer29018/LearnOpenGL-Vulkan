#version 450
#extension GL_GOOGLE_include_directive : require

#include "light.glsl"

layout (location = 0) in vec2 v_UV;
layout (location = 1) in vec4 v_Colour;
layout (location = 2) in vec3 v_Normal;
layout (location = 3) in vec3 v_FragPos;
layout (location = 4) in vec3 v_CameraPos;

layout (location = 0) out vec4 f_Colour;

layout(set=1, binding = 0) uniform sampler2D u_BoxSampler;
layout(set=1, binding = 1) uniform sampler2D u_FaceSampler;

void main()
{
    vec4 box = texture(u_BoxSampler, v_UV);
    vec4 face = texture(u_FaceSampler, v_UV);

    vec3 colour = u_Lights.ambient.rgb * u_Lights.ambient.a;

    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    vec3 norm = normalize(v_Normal);
    for (int i = 0; i < u_Lights.lightCount; i++)
    {
        vec3 lightPos = u_Lights.lights[i].position;
        vec3 lightColour = u_Lights.lights[i].colour.rgb;

        vec3 lightDir = normalize(lightPos - v_FragPos);

        float diff = max(dot(norm, lightDir), 0.0);
        diffuse += diff * lightColour;

        float specularStrength = 0.5;
        vec3 viewDir = normalize(v_CameraPos - v_FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        specular += specularStrength * spec * lightColour;
    }

    colour += diffuse;
    colour += specular;

    colour = mix(box, face, 0.5).rgb * colour.rgb;

    // f_Colour = mix(box, face, 0.5);
    f_Colour = vec4(colour, 1.0);
}
