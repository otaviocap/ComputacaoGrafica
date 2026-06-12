#include "PathAnimator.hpp"

#include <cmath>

glm::vec3 interpolateCatmullRom(const std::vector<glm::vec3>& points,
                                float progress) {
    const int pointCount = static_cast<int>(points.size());
    const int currentIndex = static_cast<int>(floor(progress)) % pointCount;
    const float localProgress = progress - floor(progress);

    const int previousIndex = (currentIndex + pointCount - 1) % pointCount;
    const int nextIndex = (currentIndex + 1) % pointCount;
    const int afterNextIndex = (currentIndex + 2) % pointCount;

    const glm::vec3 previousPoint = points[previousIndex];
    const glm::vec3 currentPoint = points[currentIndex];
    const glm::vec3 nextPoint = points[nextIndex];
    const glm::vec3 afterNextPoint = points[afterNextIndex];

    const float t2 = localProgress * localProgress;
    const float t3 = t2 * localProgress;

    return 0.5f * ((2.0f * currentPoint) +
                   (-previousPoint + nextPoint) * localProgress +
                   (2.0f * previousPoint - 5.0f * currentPoint +
                    4.0f * nextPoint - afterNextPoint) *
                       t2 +
                   (-previousPoint + 3.0f * currentPoint - 3.0f * nextPoint +
                    afterNextPoint) *
                       t3);
}

glm::vec3 objectPositionToWorldPosition(const glm::vec3& position) {
    return position;
}

void addControlPointToSelectedObject(std::vector<GameObject>& objects,
                                     int selectedModel, const Camera& camera) {
    if (selectedModel < 0 || selectedModel >= static_cast<int>(objects.size())) {
        return;
    }

    GameObject& obj = objects.at(selectedModel);
    obj.controlPoints.push_back(camera.position);
    obj.pathProgress = 0.0f;
}

void updateObjectPaths(std::vector<GameObject>& objects, float deltaTime,
                       float pathSpeed) {
    for (GameObject& obj : objects) {
        if (obj.controlPoints.size() < 4) {
            continue;
        }

        obj.pathProgress += pathSpeed * deltaTime;
        const float segmentCount = static_cast<float>(obj.controlPoints.size());
        if (obj.pathProgress >= segmentCount) {
            obj.pathProgress = fmod(obj.pathProgress, segmentCount);
        }

        obj.setPosition(
            interpolateCatmullRom(obj.controlPoints, obj.pathProgress));
    }
}
