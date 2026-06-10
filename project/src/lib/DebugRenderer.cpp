#include "DebugRenderer.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "PathAnimator.hpp"

void setupDebugGeometry(GLuint& vao, GLuint& vbo) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                          (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void drawDebugGeometry(GLuint debugProgram, const Camera& camera, GLuint vao,
                       GLuint vbo, const std::vector<glm::vec3>& positions,
                       GLenum mode, const glm::vec3& color, float pointSize,
                       float lineWidth, int viewportWidth,
                       int viewportHeight) {
    if (positions.empty()) {
        return;
    }

    glUseProgram(debugProgram);
    glUniformMatrix4fv(glGetUniformLocation(debugProgram, "view"), 1, GL_FALSE,
                       glm::value_ptr(camera.getViewMatrix()));
    glUniformMatrix4fv(
        glGetUniformLocation(debugProgram, "projection"), 1, GL_FALSE,
        glm::value_ptr(camera.getProjectionMatrix(static_cast<float>(viewportWidth),
                                                  static_cast<float>(viewportHeight))));
    glUniform3fv(glGetUniformLocation(debugProgram, "debugColor"), 1,
                 glm::value_ptr(color));

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3),
                 positions.data(), GL_DYNAMIC_DRAW);

    glPointSize(pointSize);
    glLineWidth(lineWidth);
    glDrawArrays(mode, 0, static_cast<GLsizei>(positions.size()));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void drawLightDebugPoints(GLuint debugProgram, const Camera& camera,
                          GLuint vao, GLuint vbo,
                          const std::array<LightState, MAX_LIGHTS>& lightStates,
                          int lightCount, int viewportWidth, int viewportHeight) {
    std::vector<glm::vec3> lightPositions;
    for (int i = 0; i < lightCount; ++i) {
        if (lightStates[i].enabled != 0) {
            lightPositions.push_back(lightStates[i].position);
        }
    }

    drawDebugGeometry(debugProgram, camera, vao, vbo, lightPositions, GL_POINTS,
                      glm::vec3(1.0f, 0.9f, 0.0f), 18.0f, 1.0f, viewportWidth,
                      viewportHeight);
}

void drawPathDebugLine(GLuint debugProgram, const Camera& camera, GLuint vao,
                       GLuint vbo, const std::vector<GameObject>& objects,
                       int selectedModel, int debugPathSamplesPerSegment,
                       int viewportWidth, int viewportHeight) {
    if (selectedModel < 0 || selectedModel >= static_cast<int>(objects.size())) {
        return;
    }

    const GameObject& obj = objects.at(selectedModel);
    if (obj.controlPoints.size() < 4) {
        return;
    }

    std::vector<glm::vec3> pathPositions;
    const int sampleCount =
        static_cast<int>(obj.controlPoints.size()) * debugPathSamplesPerSegment;

    for (int i = 0; i <= sampleCount; ++i) {
        const float progress =
            static_cast<float>(i) / static_cast<float>(debugPathSamplesPerSegment);
        const glm::vec3 pathPoint = interpolateCatmullRom(obj.controlPoints, progress);
        pathPositions.push_back(objectPositionToWorldPosition(pathPoint));
    }

    drawDebugGeometry(debugProgram, camera, vao, vbo, pathPositions,
                      GL_LINE_STRIP, glm::vec3(0.1f, 0.8f, 1.0f), 1.0f, 4.0f,
                      viewportWidth, viewportHeight);
}
