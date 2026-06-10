#version 400

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texc;
layout (location = 2) in vec3 normal;

uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;

out vec2 texCoord;
out vec3 fragPos;
out vec3 vNormal;

void main()
{
    vec4 worldPos = model * vec4(position, 1.0);
    fragPos = vec3(worldPos);
    
    gl_Position = projection * view * worldPos;

    texCoord = texc;
    vNormal = mat3(transpose(inverse(model))) * normal;
}
