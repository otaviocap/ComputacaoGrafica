using namespace std;
#include <assert.h>
#include <glad/glad.h>

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

using namespace glm;

struct Material {
    glm::vec3 ambient = glm::vec3(0.1f);
    glm::vec3 diffuse = glm::vec3(1.0f);
    glm::vec3 specular = glm::vec3(0.5f);
    float shininess = 32.0f;
};

string getDirectory(string filePATH) {
    size_t found = filePATH.find_last_of("/\\");
    if (found == string::npos) return "";
    return filePATH.substr(0, found + 1);
}

Material loadMaterialFromMTL(string mtlPATH, string& textureFile) {
    Material material;
    textureFile.clear();

    std::ifstream arqEntrada(mtlPATH.c_str());
    if (!arqEntrada.is_open()) {
        std::cerr << "Erro ao tentar ler o arquivo " << mtlPATH << std::endl;
        return material;
    }

    std::string line;
    while (std::getline(arqEntrada, line)) {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "Ka") {
            ssline >> material.ambient.r >> material.ambient.g >>
                material.ambient.b;
        } else if (word == "Kd") {
            ssline >> material.diffuse.r >> material.diffuse.g >>
                material.diffuse.b;
        } else if (word == "Ks") {
            ssline >> material.specular.r >> material.specular.g >>
                material.specular.b;
        } else if (word == "Ns") {
            ssline >> material.shininess;
        } else if (word == "map_Kd") {
            ssline >> textureFile;
        }
    }

    return material;
}

GLuint loadTexture(string filePATH) {
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
        stbi_load(filePATH.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = nrChannels == 4 ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                     GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Failed to load texture " << filePATH << std::endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}

int loadSimpleOBJ(string filePATH, int& nVertices, GLuint& textureId,
                  Material& material) {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    string mtlFile;
    string objDirectory = getDirectory(filePATH);

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open()) {
        std::cerr << "Erro ao tentar ler o arquivo " << filePATH << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(arqEntrada, line)) {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "v") {
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        } else if (word == "vt") {
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        } else if (word == "vn") {
            glm::vec3 normal;
            ssline >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } else if (word == "f") {
            while (ssline >> word) {
                int vi = 0, ti = -1, ni = -1;
                std::istringstream ss(word);
                std::string index;

                if (std::getline(ss, index, '/'))
                    vi = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index, '/'))
                    ti = !index.empty() ? std::stoi(index) - 1 : -1;
                if (std::getline(ss, index))
                    ni = !index.empty() ? std::stoi(index) - 1 : -1;

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
            ssline >> mtlFile;
        }
    }

    arqEntrada.close();

    if (!mtlFile.empty()) {
        string textureFile;
        material = loadMaterialFromMTL(objDirectory + mtlFile, textureFile);
        if (!textureFile.empty()) {
            textureId = loadTexture(objDirectory + textureFile);
        }
    }

    std::cout << "Gerando o buffer de geometria..." << std::endl;
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

    nVertices = vBuffer.size() / 8;  // x, y, z, s, t, nx, ny, nz por vertice

    return VAO;
}

class Transformation {
  public:
    float angleX, angleY, angleZ;
    float posX, posY, posZ;
    float scale;

    GLuint vao;
    int vertexNum;
    GLuint textureId;
    Material material;

    Transformation()
        : angleX(0.0f),
          angleY(0.0f),
          angleZ(0.0f),
          posX(0.0f),
          posY(0.0f),
          posZ(0.0f),
          scale(0.3f),
          vao(0),
          vertexNum(0),
          textureId(0) {}
};

string models[] = {
    "../assets/Modelos3D/Suzanne.obj", "../assets/Modelos3D/Suzanne.obj",
    "../assets/Modelos3D/Suzanne.obj", "../assets/Modelos3D/Suzanne.obj",
    "../assets/Modelos3D/Suzanne.obj"};
int sizeOfModels = sizeof(models) / sizeof(models[0]);

int selectedModel = 0;

std::vector<Transformation> objects = std::vector<Transformation>();

void key_callback(GLFWwindow* window, int key, int scancode, int action,
                  int mode);

int setupShader();

void handleInput();

const GLuint WIDTH = 1000, HEIGHT = 1000;

const GLchar* vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texc;
layout (location = 2) in vec3 normal;

uniform mat4 projection;
uniform mat4 model;

out vec2 texCoord;
out vec3 fragPos;
out vec3 vNormal;

void main()
{
    vec4 worldPos = model * vec4(position, 1.0);
    gl_Position = projection * worldPos;
    texCoord = texc;
    fragPos = vec3(worldPos);
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
    vec3 ambient = materialAmbient * lightColor * albedo;

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

int main() {
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(
        WIDTH, HEIGHT, "Ola 3D -- Otávio Henrique!", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    const GLubyte* renderer =
        glGetString(GL_RENDERER);                     /* get renderer string */
    const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();

    for (int i = 0; i < sizeOfModels; i++) {
        Transformation t = Transformation();
        t.vao = loadSimpleOBJ(models[i], t.vertexNum, t.textureId, t.material);
        objects.push_back(t);
    }

    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);
    glActiveTexture(GL_TEXTURE0);

    glm::vec3 lightPos(0.6, 1.2, -0.5);
    glm::vec3 lightColor(.5f, .5f, .5f);
    glm::vec3 camPos(0.0f, 0.0f, 0.0f);

    glUniform3fv(glGetUniformLocation(shaderID, "lightPos"), 1,
                 glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderID, "lightColor"), 1,
                 glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(shaderID, "camPos"), 1,
                 glm::value_ptr(camPos));

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1,
                       GL_FALSE, glm::value_ptr(projection));

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

        handleInput();

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLineWidth(10);
        glPointSize(20);

        for (int i = 0; i < (int)objects.size(); i++) {
            Transformation obj = objects.at(i);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(
                model, glm::vec3(obj.posX, obj.posY, obj.posZ - 3.0f));
            model = glm::rotate(model, glm::radians(90.0f),
                                glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, obj.angleX, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, obj.angleY, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, obj.angleZ, glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(obj.scale));

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

        glfwSwapBuffers(window);
    }

    for (int i = 0; i < (int)objects.size(); i++) {
        Transformation obj = objects.at(i);

        glDeleteVertexArrays(1, &obj.vao);
        glDeleteTextures(1, &obj.textureId);
    }

    glfwTerminate();
    return 0;
}

bool keys[1024];

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

void handleInput() {
    for (int i = 0; i < sizeOfModels; ++i) {
        if (keys[GLFW_KEY_1 + i]) {
            selectedModel = i;
        }
    }

    Transformation& t = objects.at(selectedModel);

    if (keys[GLFW_KEY_X]) {
        t.angleX += 0.1f;
    }

    if (keys[GLFW_KEY_Y]) {
        t.angleY += 0.1f;
    }

    if (keys[GLFW_KEY_Z]) {
        t.angleZ += 0.1f;
    }

    if (keys[GLFW_KEY_W]) {
        t.posZ += 0.1f;
    }

    if (keys[GLFW_KEY_A]) {
        t.posX -= 0.1f;
    }

    if (keys[GLFW_KEY_S]) {
        t.posZ -= 0.1f;
    }

    if (keys[GLFW_KEY_D]) {
        t.posX += 0.1f;
    }

    if (keys[GLFW_KEY_I]) {
        t.posY += 0.1f;
    }

    if (keys[GLFW_KEY_J]) {
        t.posY -= 0.1f;
    }

    if (keys[GLFW_KEY_LEFT_BRACKET]) {
        t.scale += 0.1f;
    }

    if (keys[GLFW_KEY_RIGHT_BRACKET]) {
        t.scale -= 0.1f;
    }
}

int setupShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}
