#include <glad.h>
//
#include <GLFW/glfw3.h>
#include <imgui.h>
//
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
//
#include <toml++/toml.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "BaseShader.hpp"
#include "Camera.hpp"
#include "DebugRenderer.hpp"
#include "GameObject.hpp"
#include "LightState.hpp"
#include "ObjLoader.hpp"
#include "PathAnimator.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace fs = std::filesystem;

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
std::vector<std::string> availableModels;
int selectedModel = -1;
int lightCount = 3;
bool showLightDebugPoints = false;
bool showPathDebugLine = false;
bool keys[1024];
float lastFrame = 0.0f;
float deltaTime = 0.0f;
bool firstMouse = true;
bool isMouseCaptured = false;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
int viewportWidth = WIDTH;
int viewportHeight = HEIGHT;
GLuint shaderID = 0;
GLuint debugShaderID = 0;
GLuint debugVAO = 0;
GLuint debugVBO = 0;
GLuint uboLights = 0;

#if defined(IMGUI_IMPL_OPENGL_ES2)
const char* glsl_version = "#version 100";
#elif defined(IMGUI_IMPL_OPENGL_ES3)
const char* glsl_version = "#version 300 es";
#elif defined(__APPLE__)
const char* glsl_version = "#version 150";
#else
const char* glsl_version = "#version 130";
#endif

std::array<LightState, MAX_LIGHTS> lightStates = {
    LightState{glm::vec3(-2.5f, 3.5f, 2.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f),
               true},
    LightState{glm::vec3(2.5f, 1.8f, 1.5f), 0.0f, glm::vec3(0.5f, 0.5f, 1.0f),
               true},
    LightState{glm::vec3(0.0f, 2.8f, -3.5f), 0.0f, glm::vec3(1.0f, 0.5f, 0.25f),
               true},
};

void toggleLight(int index) {
    lightStates[index].enabled = !lightStates[index].enabled;
}

void uploadLights() {
    glBindBuffer(GL_UNIFORM_BUFFER, uboLights);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightState) * MAX_LIGHTS,
                    lightStates.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void refreshAvailableModels() {
    availableModels.clear();
    std::string path = std::string(VIEWER_ASSETS_DIR) + "/models";
    if (!fs::exists(path)) return;

    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".obj") {
            availableModels.push_back(entry.path().string());
        }
    }
}

void addObject(const std::string& path) {
    GameObject obj;
    obj.modelPath = path;
    loadMultiMaterialOBJ(path, obj.meshes);
    objects.push_back(obj);
    selectedModel = static_cast<int>(objects.size()) - 1;
}

void unloadObjects() {
    for (auto& obj : objects) {
        for (auto& mesh : obj.meshes) {
            glDeleteVertexArrays(1, &mesh.vao);
            glDeleteTextures(1, &mesh.textureId);
        }
    }
    objects.clear();
    selectedModel = -1;
}

void saveScene(const std::string& filename) {
    toml::table tbl;

    // Camera
    toml::table cameraTbl;
    cameraTbl.insert(
        "position",
        toml::array{camera.position.x, camera.position.y, camera.position.z});
    cameraTbl.insert("yaw", camera.yaw);
    cameraTbl.insert("pitch", camera.pitch);
    cameraTbl.insert("fov", camera.fov);
    tbl.insert("camera", cameraTbl);

    // Lights
    toml::array lightsArr;
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (i >= lightCount && !lightStates[i].enabled) continue;
        toml::table lightTbl;
        lightTbl.insert("position", toml::array{lightStates[i].position.x,
                                                lightStates[i].position.y,
                                                lightStates[i].position.z});
        lightTbl.insert(
            "color", toml::array{lightStates[i].color.r, lightStates[i].color.g,
                                 lightStates[i].color.b});
        lightTbl.insert("enabled", (bool)lightStates[i].enabled);
        lightsArr.push_back(lightTbl);
    }
    tbl.insert("lights", lightsArr);

    // Objects
    toml::array objectsArr;
    for (const auto& obj : objects) {
        toml::table objTbl;
        objTbl.insert("modelPath", obj.modelPath);
        objTbl.insert("position", toml::array{obj.posX, obj.posY, obj.posZ});
        objTbl.insert("rotation",
                      toml::array{obj.angleX, obj.angleY, obj.angleZ});
        objTbl.insert("scale", obj.scale);
        objTbl.insert("pathProgress", obj.pathProgress);

        toml::array cpArr;
        for (const auto& cp : obj.controlPoints) {
            cpArr.push_back(toml::array{cp.x, cp.y, cp.z});
        }
        objTbl.insert("controlPoints", cpArr);
        objectsArr.push_back(objTbl);
    }
    tbl.insert("objects", objectsArr);

    std::ofstream file(filename);
    if (file.is_open()) {
        file << tbl;
        file.close();
        std::cout << "Scene saved to " << filename << std::endl;
    } else {
        std::cerr << "Failed to open file for saving: " << filename
                  << std::endl;
    }
}

void loadScene(const std::string& filename) {
    try {
        toml::table tbl = toml::parse_file(filename);
        unloadObjects();

        // Camera
        if (auto cameraTbl = tbl["camera"].as_table()) {
            if (auto posArr = (*cameraTbl)["position"].as_array()) {
                camera.position.x = (*posArr)[0].value_or(0.0f);
                camera.position.y = (*posArr)[1].value_or(0.0f);
                camera.position.z = (*posArr)[2].value_or(0.0f);
            }
            camera.yaw = (*cameraTbl)["yaw"].value_or(camera.yaw);
            camera.pitch = (*cameraTbl)["pitch"].value_or(camera.pitch);
            camera.fov = (*cameraTbl)["fov"].value_or(camera.fov);
            camera.updateDirection();
        }

        // Lights
        if (auto lightsArr = tbl["lights"].as_array()) {
            lightCount = std::min((int)lightsArr->size(), MAX_LIGHTS);
            for (int i = 0; i < MAX_LIGHTS; ++i) {
                if (i < lightCount) {
                    if (auto lightTbl = (*lightsArr)[i].as_table()) {
                        if (auto posArr = (*lightTbl)["position"].as_array()) {
                            lightStates[i].position.x =
                                (*posArr)[0].value_or(0.0f);
                            lightStates[i].position.y =
                                (*posArr)[1].value_or(0.0f);
                            lightStates[i].position.z =
                                (*posArr)[2].value_or(0.0f);
                        }
                        if (auto colorArr = (*lightTbl)["color"].as_array()) {
                            lightStates[i].color.r =
                                (*colorArr)[0].value_or(1.0f);
                            lightStates[i].color.g =
                                (*colorArr)[1].value_or(1.0f);
                            lightStates[i].color.b =
                                (*colorArr)[2].value_or(1.0f);
                        }
                        lightStates[i].enabled =
                            (*lightTbl)["enabled"].value_or(true) ? 1 : 0;
                    }
                } else {
                    lightStates[i].enabled = 0;
                }
            }
        }

        // Objects
        if (auto objectsArr = tbl["objects"].as_array()) {
            for (auto& objNode : *objectsArr) {
                if (auto objTbl = objNode.as_table()) {
                    std::string path = (*objTbl)["modelPath"].value_or("");
                    if (path.empty()) continue;

                    GameObject obj;
                    obj.modelPath = path;
                    loadMultiMaterialOBJ(path, obj.meshes);

                    if (auto posArr = (*objTbl)["position"].as_array()) {
                        obj.posX = (*posArr)[0].value_or(0.0f);
                        obj.posY = (*posArr)[1].value_or(0.0f);
                        obj.posZ = (*posArr)[2].value_or(0.0f);
                    }
                    if (auto rotArr = (*objTbl)["rotation"].as_array()) {
                        obj.angleX = (*rotArr)[0].value_or(0.0f);
                        obj.angleY = (*rotArr)[1].value_or(0.0f);
                        obj.angleZ = (*rotArr)[2].value_or(0.0f);
                    }
                    obj.scale = (*objTbl)["scale"].value_or(obj.scale);
                    obj.pathProgress =
                        (*objTbl)["pathProgress"].value_or(obj.pathProgress);

                    if (auto cpArr = (*objTbl)["controlPoints"].as_array()) {
                        for (auto& cpNode : *cpArr) {
                            if (auto cpSubArr = cpNode.as_array()) {
                                obj.controlPoints.push_back(
                                    glm::vec3((*cpSubArr)[0].value_or(0.0f),
                                              (*cpSubArr)[1].value_or(0.0f),
                                              (*cpSubArr)[2].value_or(0.0f)));
                            }
                        }
                    }
                    objects.push_back(obj);
                }
            }
        }
        if (!objects.empty()) {
            selectedModel = 0;
        }
        std::cout << "Scene loaded from " << filename << std::endl;
    } catch (const toml::parse_error& err) {
        std::cerr << "Failed to parse TOML: " << err << std::endl;
    }
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
        if (key == GLFW_KEY_ESCAPE) {
            isMouseCaptured = !isMouseCaptured;
            if (isMouseCaptured) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                firstMouse = true;
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

        for (int i = 0; i < MAX_LIGHTS; ++i) {
            if (key == GLFW_KEY_F1 + i) {
                toggleLight(i);
            }
        }
    }
}

void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (!isMouseCaptured) {
        return;
    }

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

    if (selectedModel < 0 ||
        selectedModel >= static_cast<int>(objects.size())) {
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

bool validateAndStartOpenGl(GLFWwindow*& window, float main_scale) {
    glfwInit();

#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // Required on Mac
#else
    // GL 3.0 + GLSL 130
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
    // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

    glfwWindowHint(GLFW_SAMPLES, 8);

    window = glfwCreateWindow(
        ScreenWidth * main_scale, ScreenHeight * main_scale,
        "Visualizador de cenas 3D - Otávio Henrique", nullptr, nullptr);
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

void setupImGui(float main_scale, GLFWwindow* window) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(
        main_scale);  // Bake a fixed style scale. (until we have a solution for
                      // dynamic style scaling, changing this requires resetting
                      // Style + calling this again)
    style.FontScaleDpi =
        main_scale;  // Set initial font scale. (in docking branch: using
                     // io.ConfigDpiScaleFonts=true automatically overrides this
                     // for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void imguiCleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
void handleImGuiFrame() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Scene Inspector");

    if (ImGui::CollapsingHeader("File", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Save Scene")) {
            saveScene(std::string(VIEWER_ASSETS_DIR) + "/scenes/scene.toml");
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Scene")) {
            loadScene(std::string(VIEWER_ASSETS_DIR) + "/scenes/scene.toml");
        }
    }

    if (ImGui::CollapsingHeader("Debug View", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Show Light Debug Points", &showLightDebugPoints);
        ImGui::Checkbox("Show Path Debug Line", &showPathDebugLine);
    }

    if (ImGui::CollapsingHeader("Objects", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::TreeNode("Asset Browser")) {
            if (ImGui::Button("Refresh Assets")) {
                refreshAvailableModels();
            }
            ImGui::Separator();
            for (const auto& model : availableModels) {
                fs::path p(model);
                if (ImGui::Button(p.filename().string().c_str())) {
                    addObject(model);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", model.c_str());
                }
            }
            ImGui::TreePop();
        }
        ImGui::Separator();

        if (ImGui::Selectable("None", selectedModel == -1)) {
            selectedModel = -1;
        }
        for (int i = 0; i < static_cast<int>(objects.size()); ++i) {
            fs::path p(objects[i].modelPath);
            std::string label = "Object " + std::to_string(i) + " (" +
                                p.filename().string() + ")";
            if (ImGui::Selectable(label.c_str(), selectedModel == i)) {
                selectedModel = i;
            }
        }

        if (selectedModel != -1 &&
            selectedModel < static_cast<int>(objects.size())) {
            if (ImGui::Button("Unload Selected Object")) {
                auto& obj = objects[selectedModel];
                for (auto& mesh : obj.meshes) {
                    glDeleteVertexArrays(1, &mesh.vao);
                    glDeleteTextures(1, &mesh.textureId);
                }
                objects.erase(objects.begin() + selectedModel);
                selectedModel = -1;
            }
        }
    }

    if (selectedModel >= 0 &&
        selectedModel < static_cast<int>(objects.size())) {
        GameObject& obj = objects.at(selectedModel);
        ImGui::Separator();
        ImGui::Text("Selected: Object %d", selectedModel);

        if (ImGui::CollapsingHeader("Transform",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Position", &obj.posX, 0.1f);
            ImGui::DragFloat("Angle X", &obj.angleX, 0.05f);
            ImGui::DragFloat("Angle Y", &obj.angleY, 0.05f);
            ImGui::DragFloat("Angle Z", &obj.angleZ, 0.05f);
            ImGui::DragFloat("Scale", &obj.scale, 0.01f, 0.001f, 10.0f);
            ImGui::Checkbox("Enable Animation", &obj.enableAnimation);
        }

        if (ImGui::CollapsingHeader("Materials",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            for (int i = 0; i < static_cast<int>(obj.meshes.size()); ++i) {
                MeshPart& mesh = obj.meshes[i];
                if (ImGui::TreeNode(mesh.name.empty()
                                        ? std::to_string(i).c_str()
                                        : mesh.name.c_str())) {
                    ImGui::ColorEdit3("Ambient",
                                      glm::value_ptr(mesh.material.ambient));
                    ImGui::ColorEdit3("Diffuse",
                                      glm::value_ptr(mesh.material.diffuse));
                    ImGui::ColorEdit3("Specular",
                                      glm::value_ptr(mesh.material.specular));
                    ImGui::SliderFloat("Shininess", &mesh.material.shininess,
                                       1.0f, 128.0f);
                    ImGui::TreePop();
                }
            }
        }
    } else {
        ImGui::Separator();
        ImGui::Text("No object selected");
    }

    if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Add Light") && lightCount < MAX_LIGHTS) {
            lightStates[lightCount] =
                LightState{glm::vec3(0.0f), 0.0f, glm::vec3(1.0f), true};
            lightCount++;
        }

        for (int i = 0; i < lightCount; ++i) {
            std::string label = "Light " + std::to_string(i);
            if (ImGui::TreeNode(label.c_str())) {
                bool enabled = lightStates[i].enabled != 0;
                if (ImGui::Checkbox("Enabled", &enabled)) {
                    lightStates[i].enabled = enabled ? 1 : 0;
                }
                ImGui::DragFloat3(
                    "Position", glm::value_ptr(lightStates[i].position), 0.1f);
                ImGui::ColorEdit3("Color",
                                  glm::value_ptr(lightStates[i].color));

                if (ImGui::Button("Remove Light")) {
                    for (int j = i; j < lightCount - 1; ++j) {
                        lightStates[j] = lightStates[j + 1];
                    }
                    lightStates[lightCount - 1].enabled = 0;
                    lightCount--;
                    ImGui::TreePop();
                    break;
                }

                ImGui::TreePop();
            }
        }
    }

    ImGui::End();
}

int main() {
    GLFWwindow* window;

    float main_scale = 1.0f;

    if (!validateAndStartOpenGl(window, main_scale)) {
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

    refreshAvailableModels();
    addObject(std::string(VIEWER_ASSETS_DIR) + "/models/casa.obj");

    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);
    glActiveTexture(GL_TEXTURE0);

    const GLint modelLoc = glGetUniformLocation(shaderID, "model");
    const GLint useTextureLoc = glGetUniformLocation(shaderID, "useTexture");
    const GLint materialAmbientLoc =
        glGetUniformLocation(shaderID, "materialAmbient");
    const GLint materialDiffuseLoc =
        glGetUniformLocation(shaderID, "materialDiffuse");
    const GLint materialSpecularLoc =
        glGetUniformLocation(shaderID, "materialSpecular");
    const GLint materialShininessLoc =
        glGetUniformLocation(shaderID, "materialShininess");

    glEnable(GL_DEPTH_TEST);

    setupImGui(main_scale, window);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        handleImGuiFrame();

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

            for (auto& mesh : obj.meshes) {
                glUniform1i(useTextureLoc, mesh.textureId != 0 ? 1 : 0);
                glUniform3fv(materialAmbientLoc, 1,
                             glm::value_ptr(mesh.material.ambient));
                glUniform3fv(materialDiffuseLoc, 1,
                             glm::value_ptr(mesh.material.diffuse));
                glUniform3fv(materialSpecularLoc, 1,
                             glm::value_ptr(mesh.material.specular));
                glUniform1f(materialShininessLoc, mesh.material.shininess);

                glBindVertexArray(mesh.vao);
                glBindTexture(GL_TEXTURE_2D, mesh.textureId);
                glDrawArrays(GL_TRIANGLES, 0, mesh.vertexNum);

                if (i == selectedModel) {
                    glDrawArrays(GL_POINTS, 0, mesh.vertexNum);
                }
            }

            glBindVertexArray(0);
        }

        if (showLightDebugPoints) {
            drawLightDebugPoints(debugShaderID, camera, debugVAO, debugVBO,
                                 lightStates, MAX_LIGHTS, viewportWidth,
                                 viewportHeight);
        }

        if (showPathDebugLine) {
            drawPathDebugLine(debugShaderID, camera, debugVAO, debugVBO,
                              objects, selectedModel,
                              DEBUG_PATH_SAMPLES_PER_SEGMENT, viewportWidth,
                              viewportHeight);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    imguiCleanup();

    glDeleteVertexArrays(1, &debugVAO);
    glDeleteBuffers(1, &debugVBO);
    glDeleteBuffers(1, &uboLights);
    glDeleteProgram(debugShaderID);
    glDeleteProgram(shaderID);

    unloadObjects();

    glfwTerminate();
    return 0;
}
