#pragma once

#include <glm/glm.hpp>

class Camera {
  public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    float yaw;
    float pitch;
    float fov;

    Camera();

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float viewportWidth,
                                  float viewportHeight) const;

    void updateDirection();
    void moveForward(float distance);
    void moveBackward(float distance);
    void moveLeft(float distance);
    void moveRight(float distance);
};
