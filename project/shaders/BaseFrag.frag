#version 400

#define MAX_LIGHTS 8

struct Light {
    vec3 position;
    vec3 color;
    int enabled;
};

in vec2 texCoord;
in vec3 fragPos;
in vec3 vNormal;

layout (std140) uniform LightBlock {
    Light lights[MAX_LIGHTS];
};

uniform sampler2D texBuff;
uniform int useTexture;
uniform vec3 camPos;
uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float materialShininess;

out vec4 color;

void main()
{
    vec3 albedo = vec3(1.0);
    if (useTexture != 0) {
        albedo = texture(texBuff, texCoord).rgb;
    }

    vec3 norm = normalize(vNormal);
    vec3 viewDir = normalize(camPos - fragPos);
    vec3 result = 0.12 * materialAmbient * albedo;

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].enabled == 0) {
            continue;
        }

        vec3 lightDir = lights[i].position - fragPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);

        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        float diff = max(dot(norm, lightDir), 0.0);

        vec3 diffuse = materialDiffuse * diff * lights[i].color * albedo * attenuation;

        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
        vec3 specular = materialSpecular * spec * lights[i].color * attenuation;

        result += diffuse + specular;
    }

    color = vec4(result, 1.0);
}
