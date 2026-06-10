#pragma once

#include <glad.h>

#include <array>
#include <glm/glm.hpp>
#include <vector>

#include "Camera.hpp"
#include "GameObject.hpp"
#include "LightState.hpp"

void setupDebugGeometry(GLuint& vao, GLuint& vbo);
void drawDebugGeometry(GLuint debugProgram, const Camera& camera, GLuint vao,
                       GLuint vbo, const std::vector<glm::vec3>& positions,
                       GLenum mode, const glm::vec3& color, float pointSize,
                       float lineWidth, int viewportWidth,
                       int viewportHeight);
void drawLightDebugPoints(GLuint debugProgram, const Camera& camera,
                          GLuint vao, GLuint vbo,
                          const std::array<LightState, MAX_LIGHTS>& lightStates,
                          int lightCount, int viewportWidth, int viewportHeight);
void drawPathDebugLine(GLuint debugProgram, const Camera& camera, GLuint vao,
                       GLuint vbo, const std::vector<GameObject>& objects,
                       int selectedModel, int debugPathSamplesPerSegment,
                       int viewportWidth, int viewportHeight);
