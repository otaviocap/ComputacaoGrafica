#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera()
    : position(0.0f, 1.5f, 5.0f),
      front(0.0f, 0.0f, -1.0f),
      up(0.0f, 1.0f, 0.0f),
      yaw(-90.0f),
      pitch(0.0f),
      fov(45.0f) {
    updateDirection();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix(float viewportWidth,
                                      float viewportHeight) const {
    return glm::perspective(glm::radians(fov),
                            viewportWidth / viewportHeight, 0.1f, 100.0f);
}

void Camera::updateDirection() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::moveForward(float distance) {
    position += distance * front;
}

void Camera::moveBackward(float distance) {
    position -= distance * front;
}

void Camera::moveLeft(float distance) {
    glm::vec3 right = glm::normalize(glm::cross(front, up));
    position -= distance * right;
}

void Camera::moveRight(float distance) {
    glm::vec3 right = glm::normalize(glm::cross(front, up));
    position += distance * right;
}
