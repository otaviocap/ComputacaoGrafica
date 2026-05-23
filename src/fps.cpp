using namespace std;
#include <assert.h>
#include <glad/glad.h>

#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <map>
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

struct Material {
    glm::vec3 ambient = glm::vec3(1.0f);
    glm::vec3 diffuse = glm::vec3(1.0f);
    glm::vec3 specular = glm::vec3(0.0f);
    float shininess = 32.0f;
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

struct Mesh {
    GLuint vao;
    int vertexCount;
    GLuint textureId;
    Material material;
};

class GameObject {
  public:
    float angleX, angleY, angleZ;
    float posX, posY, posZ;
    float scale;

    vector<Mesh> meshes;

    GameObject()
        : angleX(0.0f),
          angleY(0.0f),
          angleZ(0.0f),
          posX(0.0f),
          posY(0.0f),
          posZ(0.0f),
          scale(0.3f) {}

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
in vec2 texCoord;
in vec3 fragPos;
in vec3 vNormal;

uniform sampler2D texBuff;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 camPos;
uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float materialShininess;

out vec4 color;

void main()
{
    vec3 albedo = texture(texBuff, texCoord).rgb;
    if (albedo == vec3(0.0)) albedo = vec3(1.0);

    vec3 ambient = materialAmbient * lightColor * albedo * 0.5;

    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = materialDiffuse * diff * lightColor * albedo;

    vec3 viewDir = normalize(camPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    vec3 specular = materialSpecular * spec * lightColor;

    color = vec4(ambient + diffuse + specular, 1.0);
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

void loadOBJ(string filePath, GameObject& gameObject) {
    vector<glm::vec3> vertices;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> normals;
    map<string, Material> materials;
    map<string, GLuint> textures;
    string mtlFile;
    string objDirectory = getDirectory(filePath);

    ifstream file(filePath.c_str());
    if (!file.is_open()) {
        cerr << "Erro ao tentar ler o arquivo " << filePath << endl;
        return;
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
        } else if (word == "mtllib") {
            ss >> mtlFile;
        }
    }

    file.close();

    if (!mtlFile.empty()) {
        ifstream mtlStream((objDirectory + mtlFile).c_str());
        if (mtlStream.is_open()) {
            Material currentMaterial;
            string currentMaterialName;
            string currentTextureFile;

            while (getline(mtlStream, line)) {
                istringstream mtlSS(line);
                string mtlWord;
                mtlSS >> mtlWord;

                if (mtlWord == "newmtl") {
                    if (!currentMaterialName.empty()) {
                        materials[currentMaterialName] = currentMaterial;
                        if (!currentTextureFile.empty() &&
                            textures.find(currentMaterialName) == textures.end()) {
                            textures[currentMaterialName] =
                                loadTexture(objDirectory + currentTextureFile);
                        }
                    }
                    mtlSS >> currentMaterialName;
                    currentMaterial = Material();
                    currentTextureFile.clear();
                } else if (mtlWord == "Ka") {
                    mtlSS >> currentMaterial.ambient.r >>
                        currentMaterial.ambient.g >> currentMaterial.ambient.b;
                } else if (mtlWord == "Kd") {
                    mtlSS >> currentMaterial.diffuse.r >>
                        currentMaterial.diffuse.g >> currentMaterial.diffuse.b;
                } else if (mtlWord == "Ks") {
                    mtlSS >> currentMaterial.specular.r >>
                        currentMaterial.specular.g >>
                        currentMaterial.specular.b;
                } else if (mtlWord == "Ns") {
                    mtlSS >> currentMaterial.shininess;
                } else if (mtlWord == "map_Kd") {
                    mtlSS >> currentTextureFile;
                }
            }
            if (!currentMaterialName.empty()) {
                materials[currentMaterialName] = currentMaterial;
                if (!currentTextureFile.empty() &&
                    textures.find(currentMaterialName) == textures.end()) {
                    textures[currentMaterialName] =
                        loadTexture(objDirectory + currentTextureFile);
                }
            }
            mtlStream.close();
        }
    }

    file.open(filePath.c_str());
    string currentMaterialName = "default";
    vector<GLfloat> currentMeshBuffer;

    while (getline(file, line)) {
        istringstream ss(line);
        string word;
        ss >> word;

        if (word == "usemtl") {
            if (!currentMeshBuffer.empty()) {
                Mesh mesh;
                mesh.material = materials.count(currentMaterialName)
                                    ? materials[currentMaterialName]
                                    : Material();
                mesh.textureId = textures.count(currentMaterialName)
                                     ? textures[currentMaterialName]
                                     : 0;
                mesh.vertexCount = currentMeshBuffer.size() / 8;

                GLuint VBO, VAO;
                glGenBuffers(1, &VBO);
                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferData(GL_ARRAY_BUFFER,
                             currentMeshBuffer.size() * sizeof(GLfloat),
                             currentMeshBuffer.data(), GL_STATIC_DRAW);

                glGenVertexArrays(1, &VAO);
                glBindVertexArray(VAO);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                                      8 * sizeof(GLfloat), (GLvoid*)0);
                glEnableVertexAttribArray(0);

                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                                      8 * sizeof(GLfloat),
                                      (GLvoid*)(3 * sizeof(GLfloat)));
                glEnableVertexAttribArray(1);

                glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                                      8 * sizeof(GLfloat),
                                      (GLvoid*)(5 * sizeof(GLfloat)));
                glEnableVertexAttribArray(2);

                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);

                mesh.vao = VAO;
                gameObject.meshes.push_back(mesh);
                currentMeshBuffer.clear();
            }
            ss >> currentMaterialName;
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

                currentMeshBuffer.push_back(vertices[vi].x);
                currentMeshBuffer.push_back(vertices[vi].y);
                currentMeshBuffer.push_back(vertices[vi].z);
                currentMeshBuffer.push_back(texCoord.s);
                currentMeshBuffer.push_back(1.0f - texCoord.t);
                currentMeshBuffer.push_back(normal.x);
                currentMeshBuffer.push_back(normal.y);
                currentMeshBuffer.push_back(normal.z);
            }
        }
    }

    if (!currentMeshBuffer.empty()) {
        Mesh mesh;
        mesh.material = materials.count(currentMaterialName)
                            ? materials[currentMaterialName]
                            : Material();
        mesh.textureId = textures.count(currentMaterialName)
                             ? textures[currentMaterialName]
                             : 0;
        mesh.vertexCount = currentMeshBuffer.size() / 8;

        GLuint VBO, VAO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER,
                     currentMeshBuffer.size() * sizeof(GLfloat),
                     currentMeshBuffer.data(), GL_STATIC_DRAW);

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

        mesh.vao = VAO;
        gameObject.meshes.push_back(mesh);
    }

    file.close();
    cout << "OBJ loaded with " << gameObject.meshes.size() << " mesh groups"
         << endl;
}

Camera camera;
vector<GameObject> objects;

string models[] = {"../assets/lost-empire/lost_empire.obj"};
int sizeOfModels = sizeof(models) / sizeof(models[0]);
int selectedModel = 0;

bool keys[1024];
float lastFrame = 0.0f, deltaTime = 0.0f;
bool firstMouse = true;
float lastX = WIDTH / 2.0, lastY = HEIGHT / 2.0;

int viewportWidth = WIDTH, viewportHeight = HEIGHT;
GLuint shaderID = 0;

void key_callback(GLFWwindow* window, int key, int scancode, int action,
                  int mode) {
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
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
    for (int i = 0; i < sizeOfModels; ++i) {
        if (keys[GLFW_KEY_1 + i]) {
            selectedModel = i;
        }
    }

    float movement = CAMERA_SPEED * deltaTime;

    if (keys[GLFW_KEY_W]) camera.moveForward(movement);
    if (keys[GLFW_KEY_S]) camera.moveBackward(movement);
    if (keys[GLFW_KEY_A]) camera.moveLeft(movement);
    if (keys[GLFW_KEY_D]) camera.moveRight(movement);

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

    for (int i = 0; i < sizeOfModels; i++) {
        GameObject obj;
        loadOBJ(models[i], obj);
        objects.push_back(obj);
    }

    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);
    glActiveTexture(GL_TEXTURE0);

    glm::vec3 lightPos(5.0f, 10.0f, 5.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

    glUniform3fv(glGetUniformLocation(shaderID, "lightPos"), 1,
                 glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderID, "lightColor"), 1,
                 glm::value_ptr(lightColor));

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

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLineWidth(10);
        glPointSize(20);

        glm::mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE,
                           glm::value_ptr(view));
        glUniform3fv(glGetUniformLocation(shaderID, "camPos"), 1,
                     glm::value_ptr(camera.position));
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1,
                           GL_FALSE,
                           glm::value_ptr(camera.getProjectionMatrix(
                               (float)viewportWidth, (float)viewportHeight)));

        for (int i = 0; i < (int)objects.size(); i++) {
            GameObject& obj = objects.at(i);

            glm::mat4 model = obj.getModelMatrix();
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            for (int j = 0; j < (int)obj.meshes.size(); j++) {
                Mesh& mesh = obj.meshes[j];

                glUniform3fv(materialAmbientLoc, 1,
                             glm::value_ptr(mesh.material.ambient));
                glUniform3fv(materialDiffuseLoc, 1,
                             glm::value_ptr(mesh.material.diffuse));
                glUniform3fv(materialSpecularLoc, 1,
                             glm::value_ptr(mesh.material.specular));
                glUniform1f(materialShininessLoc, mesh.material.shininess);

                glBindVertexArray(mesh.vao);
                glBindTexture(GL_TEXTURE_2D, mesh.textureId);
                glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);

                // if (i == selectedModel) {
                //     glDrawArrays(GL_POINTS, 0, mesh.vertexCount);
                // }
            }

            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }

    for (int i = 0; i < (int)objects.size(); i++) {
        for (int j = 0; j < (int)objects[i].meshes.size(); j++) {
            glDeleteVertexArrays(1, &objects[i].meshes[j].vao);
            glDeleteTextures(1, &objects[i].meshes[j].textureId);
        }
    }

    glfwTerminate();
    return 0;
}
