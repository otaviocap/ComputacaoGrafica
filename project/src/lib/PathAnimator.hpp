#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "Camera.hpp"
#include "GameObject.hpp"

glm::vec3 interpolateCatmullRom(const std::vector<glm::vec3>& points,
                                float progress);
glm::vec3 objectPositionToWorldPosition(const glm::vec3& position);
void addControlPointToSelectedObject(std::vector<GameObject>& objects,
                                     int selectedModel, const Camera& camera);
void updateObjectPaths(std::vector<GameObject>& objects, float deltaTime,
                       float pathSpeed);
