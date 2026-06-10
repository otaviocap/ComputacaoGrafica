using namespace std;
#include <assert.h>
#include <glad/glad.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmath>

const GLuint WIDTH = 1000, HEIGHT = 1000;
const float CAMERA_SPEED = 3.0f;
const float SENSITIVITY = 0.05f;
const float ROTATION_SPEED = 10.0f;
const float PATH_SPEED = 0.6f;
const int DEBUG_PATH_SAMPLES_PER_SEGMENT = 20;

struct Material {
    glm::vec3 ambient = glm::vec3(0.1f);
    glm::vec3 diffuse = glm::vec3(1.0f);
    glm::vec3 specular = glm::vec3(0.5f);
    float shininess = 32.0f;
};

constexpr int MAX_LIGHTS = 8;

struct LightState {
    glm::vec3 position;  // 12 bytes
    float padding1;      // 4 bytes

    glm::vec3 color;  // 12 bytes
    int enabled;      // 4 bytes
};

class Camera {
  public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    float yaw;
    float pitch;
    float fov;

    Camera()
        : position(0.0f, 0.0f, 0.0f),
          front(0.0f, 0.0f, -1.0f),
          up(0.0f, 1.0f, 0.0f),
          yaw(-90.0f),
          pitch(0.0f),
          fov(45) {}

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 getProjectionMatrix(float viewportWidth,
                                  float viewportHeight) const {
        return glm::perspective(glm::radians(fov),
                                (float)viewportWidth / (float)viewportHeight,
                                0.1f, 100.0f);
    }

    void updateDirection() {
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);

        glm::vec3 right =
            glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        up = glm::normalize(glm::cross(right, front));
    }

    void moveForward(float distance) { position += distance * front; }
    void moveBackward(float distance) { position -= distance * front; }
    void moveLeft(float distance) {
        glm::vec3 right = glm::normalize(glm::cross(front, up));
        position -= distance * right;
    }
    void moveRight(float distance) {
        glm::vec3 right = glm::normalize(glm::cross(front, up));
        position += distance * right;
    }
};

class GameObject {
  public:
    float angleX, angleY, angleZ;
    float posX, posY, posZ;
    float scale;
    float pathProgress;
    vector<glm::vec3> controlPoints;

    GLuint vao;
    int vertexNum;
    GLuint textureId;
    Material material;

    GameObject()
        : angleX(0.0f),
          angleY(0.0f),
          angleZ(0.0f),
          posX(0.0f),
          posY(0.0f),
          posZ(0.0f),
          scale(0.3f),
          pathProgress(0.0f),
          vao(0),
          vertexNum(0),
          textureId(0) {}

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

const GLchar* vertexShaderSource = R"(
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
})";

const GLchar* fragmentShaderSource = R"(
#version 400
#define MAX_LIGHTS 8
struct Light {
    vec3 position;
    vec3 color;
    int enabled;
};

in vec2 texCoord;
in vec3 fragPos;
in vec3 vNormal;

layout (std140) uniform LightBlock {
    Light lights[MAX_LIGHTS];
};

uniform sampler2D texBuff;
uniform vec3 camPos;
uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float materialShininess;

out vec4 color;

void main()
{
    vec3 albedo = texture(texBuff, texCoord).rgb;
    vec3 norm = normalize(vNormal);
    vec3 viewDir = normalize(camPos - fragPos);
    vec3 result = 0.12 * materialAmbient * albedo;

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lights[i].enabled == 0) {
            continue;
        }
    
        vec3 lightDir = lights[i].position - fragPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);
        
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        float diff = max(dot(norm, lightDir), 0.0);
        
        vec3 diffuse = materialDiffuse * diff * lights[i].color * albedo * attenuation;

        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
        vec3 specular = materialSpecular * spec * lights[i].color * attenuation;

        result += diffuse + specular;
    }

    color = vec4(result, 1.0);
})";

const GLchar* debugVertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    gl_Position = projection * view * vec4(position, 1.0);
})";

const GLchar* debugFragmentShaderSource = R"(
#version 400
uniform vec3 debugColor;

out vec4 color;

void main()
{
    color = vec4(debugColor, 1.0);
})";

GLuint setupShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
             << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
             << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint setupDebugShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &debugVertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::DEBUG_VERTEX::COMPILATION_FAILED\n"
             << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &debugFragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::DEBUG_FRAGMENT::COMPILATION_FAILED\n"
             << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER::DEBUG_PROGRAM::LINKING_FAILED\n"
             << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

string getDirectory(string filePath) {
    size_t found = filePath.find_last_of("/\\");
    if (found == string::npos) return "";
    return filePath.substr(0, found + 1);
}

Material loadMaterialFromMTL(string mtlPath, string& textureFile) {
    Material material;
    textureFile.clear();

    ifstream file(mtlPath.c_str());
    if (!file.is_open()) {
        cerr << "Erro ao tentar ler o arquivo " << mtlPath << endl;
        return material;
    }

    string line;
    while (getline(file, line)) {
        istringstream ss(line);
        string word;
        ss >> word;

        if (word == "Ka") {
            ss >> material.ambient.r >> material.ambient.g >>
                material.ambient.b;
        } else if (word == "Kd") {
            ss >> material.diffuse.r >> material.diffuse.g >>
                material.diffuse.b;
        } else if (word == "Ks") {
            ss >> material.specular.r >> material.specular.g >>
                material.specular.b;
        } else if (word == "Ns") {
            ss >> material.shininess;
        } else if (word == "map_Kd") {
            ss >> textureFile;
        }
    }

    return material;
}

GLuint loadTexture(string filePath) {
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data =
        stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = nrChannels == 4 ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                     GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        cout << "Failed to load texture " << filePath << endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}

int loadSimpleOBJ(string filePath, int& nVertices, GLuint& textureId,
                  Material& material) {
    vector<glm::vec3> vertices;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> normals;
    vector<GLfloat> vBuffer;
    string mtlFile;
    string objDirectory = getDirectory(filePath);

    ifstream file(filePath.c_str());
    if (!file.is_open()) {
        cerr << "Erro ao tentar ler o arquivo " << filePath << endl;
        return -1;
    }

    string line;
    while (getline(file, line)) {
        istringstream ss(line);
        string word;
        ss >> word;

        if (word == "v") {
            glm::vec3 vertex;
            ss >> vertex.x >> vertex.y >> vertex.z;
            vertices.push_back(vertex);
        } else if (word == "vt") {
            glm::vec2 texCoord;
            ss >> texCoord.s >> texCoord.t;
            texCoords.push_back(texCoord);
        } else if (word == "vn") {
            glm::vec3 normal;
            ss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } else if (word == "f") {
            while (ss >> word) {
                int vi = 0, ti = -1, ni = -1;
                istringstream ssIndex(word);
                string index;

                if (getline(ssIndex, index, '/'))
                    vi = !index.empty() ? stoi(index) - 1 : 0;
                if (getline(ssIndex, index, '/'))
                    ti = !index.empty() ? stoi(index) - 1 : -1;
                if (getline(ssIndex, index))
                    ni = !index.empty() ? stoi(index) - 1 : -1;

                glm::vec2 texCoord = ti >= 0 && ti < (int)texCoords.size()
                                         ? texCoords[ti]
                                         : glm::vec2(0.0f, 0.0f);
                glm::vec3 normal = ni >= 0 && ni < (int)normals.size()
                                       ? normals[ni]
                                       : glm::vec3(0.0f, 0.0f, 1.0f);

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                vBuffer.push_back(texCoord.s);
                vBuffer.push_back(1.0f - texCoord.t);
                vBuffer.push_back(normal.x);
                vBuffer.push_back(normal.y);
                vBuffer.push_back(normal.z);
            }
        } else if (word == "mtllib") {
            ss >> mtlFile;
        }
    }

    file.close();

    if (!mtlFile.empty()) {
        string textureFile;
        material = loadMaterialFromMTL(objDirectory + mtlFile, textureFile);
        if (!textureFile.empty()) {
            textureId = loadTexture(objDirectory + textureFile);
        }
    }

    cout << "Gerando o buffer de geometria..." << endl;
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat),
                 vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
                          (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
                          (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat),
                          (GLvoid*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 8;

    return VAO;
}

Camera camera;
vector<GameObject> objects;

string models[] = {"../assets/Modelos3D/Suzanne.obj",
                   "../assets/Modelos3D/Suzanne.obj",
                   "../assets/Modelos3D/Suzanne.obj"};

int sizeOfModels = sizeof(models) / sizeof(models[0]);

int selectedModel = 0;

bool keys[1024];
float lastFrame = 0.0f, deltaTime = 0.0f;
bool firstMouse = true;
float lastX = WIDTH / 2.0, lastY = HEIGHT / 2.0;

int viewportWidth = WIDTH, viewportHeight = HEIGHT;
GLuint shaderID = 0;
GLuint debugShaderID = 0;
GLuint debugVAO = 0;
GLuint debugVBO = 0;
GLuint uboLights;
const int mainObjectIndex = 0;
int lightCount = 3;
bool showLightDebugPoints = false;
bool showPathDebugLine = false;

std::array<LightState, MAX_LIGHTS> lightStates = {
    {{glm::vec3(-2.5f, 3.5f, 2.0f), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f), true},
     {glm::vec3(2.5f, 1.8f, 1.5f), 0.0f, glm::vec3(0.5f, 0.5f, 1.0f), true},
     {glm::vec3(0.0f, 2.8f, -3.5f), 0.0f, glm::vec3(1.0f, 0.5f, 0.25f), true}}};

void toggleLight(int index) {
    lightStates[index].enabled = !lightStates[index].enabled;
}

glm::vec3 interpolateCatmullRom(const vector<glm::vec3>& points,
                                float progress) {
    int pointCount = points.size();
    int currentIndex = (int)floor(progress) % pointCount;
    float localProgress = progress - floor(progress);

    int previousIndex = (currentIndex + pointCount - 1) % pointCount;
    int nextIndex = (currentIndex + 1) % pointCount;
    int afterNextIndex = (currentIndex + 2) % pointCount;

    glm::vec3 previousPoint = points[previousIndex];
    glm::vec3 currentPoint = points[currentIndex];
    glm::vec3 nextPoint = points[nextIndex];
    glm::vec3 afterNextPoint = points[afterNextIndex];

    float t2 = localProgress * localProgress;
    float t3 = t2 * localProgress;

    return 0.5f * ((2.0f * currentPoint) +
                   (-previousPoint + nextPoint) * localProgress +
                   (2.0f * previousPoint - 5.0f * currentPoint +
                    4.0f * nextPoint - afterNextPoint) *
                       t2 +
                   (-previousPoint + 3.0f * currentPoint - 3.0f * nextPoint +
                    afterNextPoint) *
                       t3);
}

void addControlPointToSelectedObject() {
    if (selectedModel < 0 || selectedModel >= (int)objects.size()) {
        return;
    }

    glm::vec3 loc = camera.position;

    GameObject& obj = objects.at(selectedModel);
    obj.controlPoints.push_back(loc);
    obj.pathProgress = 0.0f;
}

void updateObjectPaths() {
    for (GameObject& obj : objects) {
        if (obj.controlPoints.size() < 4) {
            continue;
        }

        obj.pathProgress += PATH_SPEED * deltaTime;
        float segmentCount = (float)obj.controlPoints.size();
        if (obj.pathProgress >= segmentCount) {
            obj.pathProgress = fmod(obj.pathProgress, segmentCount);
        }

        obj.setPosition(
            interpolateCatmullRom(obj.controlPoints, obj.pathProgress));
    }
}

glm::vec3 objectPositionToWorldPosition(const glm::vec3& position) {
    return glm::vec3(position.x, position.y, position.z - 3.0f);
}

void drawDebugGeometry(const vector<glm::vec3>& positions, GLenum mode,
                       const glm::vec3& color, float pointSize,
                       float lineWidth) {
    if (positions.empty()) {
        return;
    }

    glUseProgram(debugShaderID);
    glUniformMatrix4fv(glGetUniformLocation(debugShaderID, "view"), 1, GL_FALSE,
                       glm::value_ptr(camera.getViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(debugShaderID, "projection"), 1,
                       GL_FALSE,
                       glm::value_ptr(camera.getProjectionMatrix(
                           (float)viewportWidth, (float)viewportHeight)));
    glUniform3fv(glGetUniformLocation(debugShaderID, "debugColor"), 1,
                 glm::value_ptr(color));

    glBindVertexArray(debugVAO);
    glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3),
                 positions.data(), GL_DYNAMIC_DRAW);

    glPointSize(pointSize);
    glLineWidth(lineWidth);
    glDrawArrays(mode, 0, (GLsizei)positions.size());

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void drawLightDebugPoints() {
    vector<glm::vec3> lightPositions;
    for (int i = 0; i < lightCount; ++i) {
        if (lightStates[i].enabled) {
            lightPositions.push_back(lightStates[i].position);
        }
    }

    drawDebugGeometry(lightPositions, GL_POINTS, glm::vec3(1.0f, 0.9f, 0.0f),
                      18.0f, 1.0f);
}

void drawPathDebugLine() {
    if (selectedModel < 0 || selectedModel >= (int)objects.size()) {
        return;
    }

    GameObject& obj = objects.at(selectedModel);
    if (obj.controlPoints.size() < 4) {
        return;
    }

    vector<glm::vec3> pathPositions;
    int sampleCount =
        (int)obj.controlPoints.size() * DEBUG_PATH_SAMPLES_PER_SEGMENT;

    for (int i = 0; i <= sampleCount; ++i) {
        float progress = (float)i / (float)DEBUG_PATH_SAMPLES_PER_SEGMENT;
        glm::vec3 pathPoint =
            interpolateCatmullRom(obj.controlPoints, progress);
        pathPositions.push_back(objectPositionToWorldPosition(pathPoint));
    }

    drawDebugGeometry(pathPositions, GL_LINE_STRIP, glm::vec3(0.1f, 0.8f, 1.0f),
                      1.0f, 4.0f);
}

void uploadLights() {
    glBindBuffer(GL_UNIFORM_BUFFER, uboLights);

    // Enviamos o bloco inteiro de dados de luzes ativas de uma única vez!
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightState) * lightCount,
                    lightStates.data());

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action,
                  int mode) {
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }

    if (action == GLFW_PRESS) {
        for (int i = 0; i < sizeOfModels; ++i) {
            if (key == GLFW_KEY_1 + i) {
                selectedModel = i == selectedModel ? -1 : i;
            }
        }

        if (key == GLFW_KEY_SPACE) {
            addControlPointToSelectedObject();
        }

        if (key == GLFW_KEY_L) {
            showLightDebugPoints = !showLightDebugPoints;
            cout << "Debug das luzes "
                 << (showLightDebugPoints ? "ativado" : "desativado") << endl;
        }

        if (key == GLFW_KEY_P) {
            showPathDebugLine = !showPathDebugLine;
            cout << "Debug do caminho "
                 << (showPathDebugLine ? "ativado" : "desativado") << endl;
        }

        for (int i = 0; i < lightCount; ++i) {
            if (key == GLFW_KEY_F1 + i) {
                toggleLight(i);
            }
        }
    }

    if (keys[GLFW_KEY_ESCAPE]) glfwSetWindowShouldClose(window, GL_TRUE);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    viewportWidth = width;
    viewportHeight = height;
    glViewport(0, 0, width, height);

    glm::mat4 projection = camera.getProjectionMatrix(width, height);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1,
                       GL_FALSE, glm::value_ptr(projection));
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xoffset *= SENSITIVITY;
    yoffset *= SENSITIVITY;

    camera.yaw += xoffset;
    camera.pitch += yoffset;

    if (camera.pitch > 89.0f) camera.pitch = 89.0f;
    if (camera.pitch < -89.0f) camera.pitch = -89.0f;

    camera.updateDirection();
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (camera.fov >= 1.0f && camera.fov <= 45.0f) camera.fov -= yoffset;
    if (camera.fov <= 1.0f) camera.fov = 1.0f;
    if (camera.fov >= 45.0f) camera.fov = 45.0f;
}

void handleInput() {
    float movement = CAMERA_SPEED * deltaTime;

    if (keys[GLFW_KEY_W]) camera.moveForward(movement);
    if (keys[GLFW_KEY_S]) camera.moveBackward(movement);
    if (keys[GLFW_KEY_A]) camera.moveLeft(movement);
    if (keys[GLFW_KEY_D]) camera.moveRight(movement);

    if (selectedModel < 0) {
        return;
    }

    GameObject& obj = objects.at(selectedModel);
    float step = ROTATION_SPEED * deltaTime;

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

int main() {
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(
        WIDTH, HEIGHT, "Ola 3D -- Otávio Henrique!", nullptr, nullptr);

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Failed to initialize GLAD" << endl;
    }

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;

    glfwGetFramebufferSize(window, &viewportWidth, &viewportHeight);
    glViewport(0, 0, viewportWidth, viewportHeight);

    shaderID = setupShader();
    debugShaderID = setupDebugShader();

    glGenVertexArrays(1, &debugVAO);
    glGenBuffers(1, &debugVBO);
    glBindVertexArray(debugVAO);
    glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                          (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenBuffers(1, &uboLights);
    glBindBuffer(GL_UNIFORM_BUFFER, uboLights);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(LightState) * MAX_LIGHTS, nullptr,
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboLights);
    GLuint blockIndex = glGetUniformBlockIndex(shaderID, "LightBlock");
    glUniformBlockBinding(shaderID, blockIndex, 0);

    for (int i = 0; i < sizeOfModels; i++) {
        GameObject obj;
        obj.vao = loadSimpleOBJ(models[i], obj.vertexNum, obj.textureId,
                                obj.material);
        objects.push_back(obj);
    }

    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);
    glActiveTexture(GL_TEXTURE0);

    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint materialAmbientLoc =
        glGetUniformLocation(shaderID, "materialAmbient");
    GLint materialDiffuseLoc =
        glGetUniformLocation(shaderID, "materialDiffuse");
    GLint materialSpecularLoc =
        glGetUniformLocation(shaderID, "materialSpecular");
    GLint materialShininessLoc =
        glGetUniformLocation(shaderID, "materialShininess");

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        handleInput();
        updateObjectPaths();

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLineWidth(10);
        glPointSize(20);

        glUseProgram(shaderID);
        glm::mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE,
                           glm::value_ptr(view));
        glUniform3fv(glGetUniformLocation(shaderID, "camPos"), 1,
                     glm::value_ptr(camera.position));
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1,
                           GL_FALSE,
                           glm::value_ptr(camera.getProjectionMatrix(
                               (float)viewportWidth, (float)viewportHeight)));

        uploadLights();

        for (int i = 0; i < (int)objects.size(); i++) {
            GameObject& obj = objects.at(i);

            glm::mat4 model = obj.getModelMatrix();
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
            drawLightDebugPoints();
        }

        if (showPathDebugLine) {
            drawPathDebugLine();
        }

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &debugVAO);
    glDeleteBuffers(1, &debugVBO);
    glDeleteBuffers(1, &uboLights);
    glDeleteProgram(debugShaderID);
    glDeleteProgram(shaderID);

    for (int i = 0; i < (int)objects.size(); i++) {
        glDeleteVertexArrays(1, &objects[i].vao);
        glDeleteTextures(1, &objects[i].textureId);
    }

    glfwTerminate();
    return 0;
}
