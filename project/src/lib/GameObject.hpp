#pragma once

#include <glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

#include "Material.hpp"

struct MeshPart {
    GLuint vao = 0;
    int vertexNum = 0;
    GLuint textureId = 0;
    Material material;
    std::string name;
};

class GameObject {
  public:
    float angleX = 0.0f;
    float angleY = 0.0f;
    float angleZ = 0.0f;
    float posX = 0.0f;
    float posY = 0.0f;
    float posZ = 0.0f;
    float scale = 0.3f;
    float pathProgress = 0.0f;
    std::vector<glm::vec3> controlPoints;

    std::vector<MeshPart> meshes;

    glm::vec3 getPosition() const { return glm::vec3(posX, posY, posZ); }

    void setPosition(const glm::vec3& position) {
        posX = position.x;
        posY = position.y;
        posZ = position.z;
    }

    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(posX, posY, posZ - 3.0f));
        model = glm::rotate(model, glm::radians(90.0f),
                            glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, angleX, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, angleY, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, angleZ, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(scale));
        return model;
    }
};
