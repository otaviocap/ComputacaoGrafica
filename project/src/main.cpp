#include <glad.h>

#include <array>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "BaseShader.hpp"
#include "Camera.hpp"
#include "DebugRenderer.hpp"
#include "GLFW/glfw3.h"
#include "GameObject.hpp"
#include "LightState.hpp"
#include "ObjLoader.hpp"
#include "PathAnimator.hpp"
#include "glm/gtc/type_ptr.hpp"

constexpr GLuint WIDTH = 1000;
constexpr GLuint HEIGHT = 1000;
constexpr float CAMERA_SPEED = 3.0f;
constexpr float SENSITIVITY = 0.05f;
constexpr float ROTATION_SPEED = 10.0f;
constexpr float PATH_SPEED = 0.6f;
constexpr int DEBUG_PATH_SAMPLES_PER_SEGMENT = 20;

int ScreenWidth = WIDTH;
int ScreenHeight = HEIGHT;

Camera camera;
std::vector<GameObject> objects;
int selectedModel = 0;
int lightCount = 3;
bool showLightDebugPoints = false;
bool showPathDebugLine = false;
bool keys[1024];
float lastFrame = 0.0f;
float deltaTime = 0.0f;
bool firstMouse = true;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
int viewportWidth = WIDTH;
int viewportHeight = HEIGHT;
GLuint shaderID = 0;
GLuint debugShaderID = 0;
GLuint debugVAO = 0;
GLuint debugVBO = 0;
GLuint uboLights = 0;

std::array<LightState, MAX_LIGHTS> lightStates = {
    LightState{glm::vec3(-2.5f, 3.5f, 2.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f),
               true},
    LightState{glm::vec3(2.5f, 1.8f, 1.5f), 0.0f, glm::vec3(0.5f, 0.5f, 1.0f),
               true},
    LightState{glm::vec3(0.0f, 2.8f, -3.5f), 0.0f, glm::vec3(1.0f, 0.5f, 0.25f),
               true},
};

const std::array<std::string, 3> modelPaths = {
    std::string(VIEWER_ASSETS_DIR) + "/Modelos3D/Suzanne.obj",
    std::string(VIEWER_ASSETS_DIR) + "/Modelos3D/Suzanne.obj",
    std::string(VIEWER_ASSETS_DIR) + "/Modelos3D/Suzanne.obj",
};

void toggleLight(int index) {
    lightStates[index].enabled = !lightStates[index].enabled;
}

void uploadLights() {
    glBindBuffer(GL_UNIFORM_BUFFER, uboLights);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightState) * lightCount,
                    lightStates.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    ScreenWidth = width;
    ScreenHeight = height;
    viewportWidth = width;
    viewportHeight = height;
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int, int action, int) {
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            keys[key] = true;
        } else if (action == GLFW_RELEASE) {
            keys[key] = false;
        }
    }

    if (action == GLFW_PRESS) {
        for (int i = 0; i < static_cast<int>(modelPaths.size()); ++i) {
            if (key == GLFW_KEY_1 + i) {
                selectedModel = i == selectedModel ? -1 : i;
            }
        }

        if (key == GLFW_KEY_SPACE) {
            addControlPointToSelectedObject(objects, selectedModel, camera);
        }

        if (key == GLFW_KEY_L) {
            showLightDebugPoints = !showLightDebugPoints;
            std::cout << "Debug das luzes "
                      << (showLightDebugPoints ? "ativado" : "desativado")
                      << std::endl;
        }

        if (key == GLFW_KEY_P) {
            showPathDebugLine = !showPathDebugLine;
            std::cout << "Debug do caminho "
                      << (showPathDebugLine ? "ativado" : "desativado")
                      << std::endl;
        }

        for (int i = 0; i < lightCount; ++i) {
            if (key == GLFW_KEY_F1 + i) {
                toggleLight(i);
            }
        }
    }

    if (keys[GLFW_KEY_ESCAPE]) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    xoffset *= SENSITIVITY;
    yoffset *= SENSITIVITY;

    camera.yaw += xoffset;
    camera.pitch += yoffset;

    if (camera.pitch > 89.0f) camera.pitch = 89.0f;
    if (camera.pitch < -89.0f) camera.pitch = -89.0f;

    camera.updateDirection();
}

void scroll_callback(GLFWwindow*, double, double yoffset) {
    if (camera.fov >= 1.0f && camera.fov <= 45.0f) camera.fov -= yoffset;
    if (camera.fov <= 1.0f) camera.fov = 1.0f;
    if (camera.fov >= 45.0f) camera.fov = 45.0f;
}

void handleInput() {
    const float movement = CAMERA_SPEED * deltaTime;

    if (keys[GLFW_KEY_W]) camera.moveForward(movement);
    if (keys[GLFW_KEY_S]) camera.moveBackward(movement);
    if (keys[GLFW_KEY_A]) camera.moveLeft(movement);
    if (keys[GLFW_KEY_D]) camera.moveRight(movement);

    if (selectedModel < 0) {
        return;
    }

    GameObject& obj = objects.at(selectedModel);
    const float step = ROTATION_SPEED * deltaTime;

    if (keys[GLFW_KEY_X]) obj.angleX += step;
    if (keys[GLFW_KEY_Y]) obj.angleY += step;
    if (keys[GLFW_KEY_Z]) obj.angleZ += step;

    if (keys[GLFW_KEY_UP]) obj.posZ += step;
    if (keys[GLFW_KEY_LEFT]) obj.posX -= step;
    if (keys[GLFW_KEY_DOWN]) obj.posZ -= step;
    if (keys[GLFW_KEY_RIGHT]) obj.posX += step;

    if (keys[GLFW_KEY_I]) obj.posY += step;
    if (keys[GLFW_KEY_J]) obj.posY -= step;

    if (keys[GLFW_KEY_LEFT_BRACKET]) obj.scale += step;
    if (keys[GLFW_KEY_RIGHT_BRACKET]) obj.scale -= step;
}

bool validateAndStartOpenGl(GLFWwindow*& window) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8);

    window =
        glfwCreateWindow(ScreenWidth, ScreenHeight, "Paths", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glViewport(0, 0, ScreenWidth, ScreenHeight);
    return true;
}

void loadObjects() {
    for (const std::string& modelPath : modelPaths) {
        GameObject obj;
        obj.vao = loadSimpleOBJ(modelPath, obj.vertexNum, obj.textureId,
                                obj.material);
        objects.push_back(obj);
    }
}

int main() {
    GLFWwindow* window;
    if (!validateAndStartOpenGl(window)) {
        return -1;
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "Renderer: " << renderer << std::endl;
    std::cout << "OpenGL version supported " << version << std::endl;

    glfwGetFramebufferSize(window, &viewportWidth, &viewportHeight);
    glViewport(0, 0, viewportWidth, viewportHeight);

    shaderID =
        createShaderProgram(std::string(VIEWER_SHADER_DIR) + "/BaseVert.vert",
                            std::string(VIEWER_SHADER_DIR) + "/BaseFrag.frag");
    debugShaderID =
        createShaderProgram(std::string(VIEWER_SHADER_DIR) + "/DebugVert.vert",
                            std::string(VIEWER_SHADER_DIR) + "/DebugFrag.frag");

    setupDebugGeometry(debugVAO, debugVBO);

    glGenBuffers(1, &uboLights);
    glBindBuffer(GL_UNIFORM_BUFFER, uboLights);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(LightState) * MAX_LIGHTS, nullptr,
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboLights);
    const GLuint blockIndex = glGetUniformBlockIndex(shaderID, "LightBlock");
    glUniformBlockBinding(shaderID, blockIndex, 0);

    loadObjects();

    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);
    glActiveTexture(GL_TEXTURE0);

    const GLint modelLoc = glGetUniformLocation(shaderID, "model");
    const GLint materialAmbientLoc =
        glGetUniformLocation(shaderID, "materialAmbient");
    const GLint materialDiffuseLoc =
        glGetUniformLocation(shaderID, "materialDiffuse");
    const GLint materialSpecularLoc =
        glGetUniformLocation(shaderID, "materialSpecular");
    const GLint materialShininessLoc =
        glGetUniformLocation(shaderID, "materialShininess");

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        handleInput();
        updateObjectPaths(objects, deltaTime, PATH_SPEED);

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLineWidth(10);
        glPointSize(20);

        glUseProgram(shaderID);
        const glm::mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE,
                           glm::value_ptr(view));
        glUniform3fv(glGetUniformLocation(shaderID, "camPos"), 1,
                     glm::value_ptr(camera.position));
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1,
                           GL_FALSE,
                           glm::value_ptr(camera.getProjectionMatrix(
                               static_cast<float>(viewportWidth),
                               static_cast<float>(viewportHeight))));

        uploadLights();

        for (int i = 0; i < static_cast<int>(objects.size()); ++i) {
            GameObject& obj = objects.at(i);

            const glm::mat4 model = obj.getModelMatrix();
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(materialAmbientLoc, 1,
                         glm::value_ptr(obj.material.ambient));
            glUniform3fv(materialDiffuseLoc, 1,
                         glm::value_ptr(obj.material.diffuse));
            glUniform3fv(materialSpecularLoc, 1,
                         glm::value_ptr(obj.material.specular));
            glUniform1f(materialShininessLoc, obj.material.shininess);

            glBindVertexArray(obj.vao);
            glBindTexture(GL_TEXTURE_2D, obj.textureId);
            glDrawArrays(GL_TRIANGLES, 0, obj.vertexNum);

            if (i == selectedModel) {
                glDrawArrays(GL_POINTS, 0, obj.vertexNum);
            }

            glBindVertexArray(0);
        }

        if (showLightDebugPoints) {
            drawLightDebugPoints(debugShaderID, camera, debugVAO, debugVBO,
                                 lightStates, lightCount, viewportWidth,
                                 viewportHeight);
        }

        if (showPathDebugLine) {
            drawPathDebugLine(debugShaderID, camera, debugVAO, debugVBO,
                              objects, selectedModel,
                              DEBUG_PATH_SAMPLES_PER_SEGMENT, viewportWidth,
                              viewportHeight);
        }

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &debugVAO);
    glDeleteBuffers(1, &debugVBO);
    glDeleteBuffers(1, &uboLights);
    glDeleteProgram(debugShaderID);
    glDeleteProgram(shaderID);

    for (GameObject& obj : objects) {
        glDeleteVertexArrays(1, &obj.vao);
        glDeleteTextures(1, &obj.textureId);
    }

    glfwTerminate();
    return 0;
}
