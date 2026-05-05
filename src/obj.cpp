
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

GLuint textureID = 0;

string getDirectory(string filePATH) {
    size_t found = filePATH.find_last_of("/\\");
    if (found == string::npos) return "";
    return filePATH.substr(0, found + 1);
}

string loadTextureNameFromMTL(string mtlPATH) {
    std::ifstream arqEntrada(mtlPATH.c_str());
    if (!arqEntrada.is_open()) {
        std::cerr << "Erro ao tentar ler o arquivo " << mtlPATH << std::endl;
        return "";
    }

    std::string line;
    while (std::getline(arqEntrada, line)) {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "map_Kd") {
            string textureFile;
            ssline >> textureFile;
            return textureFile;
        }
    }

    return "";
}

GLuint loadTexture(string filePATH) {
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
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

int loadSimpleOBJ(string filePATH, int& nVertices) {
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
                int vi = 0, ti = 0, ni = 0;
                std::istringstream ss(word);
                std::string index;

                if (std::getline(ss, index, '/'))
                    vi = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index, '/'))
                    ti = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index))
                    ni = !index.empty() ? std::stoi(index) - 1 : 0;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                vBuffer.push_back(texCoords[ti].s);
                vBuffer.push_back(1.0f - texCoords[ti].t);
            }
        } else if (word == "mtllib") {
            ssline >> mtlFile;
        }
    }

    arqEntrada.close();

    if (!mtlFile.empty()) {
        string textureFile = loadTextureNameFromMTL(objDirectory + mtlFile);
        if (!textureFile.empty()) {
            textureID = loadTexture(objDirectory + textureFile);
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices =
        vBuffer.size() /
        5;  // x, y, z, s, t (valores armazenados por vértice)

    return VAO;
}

class Transformation {
  public:
    float angleX, angleY, angleZ;
    float posX, posY, posZ;
    float scale;

    Transformation()
        : angleX(0.0f),
          angleY(0.0f),
          angleZ(0.0f),
          posX(0.0f),
          posY(0.0f),
          posZ(0.0f),
          scale(1.0f) {}
};

void key_callback(GLFWwindow* window, int key, int scancode, int action,
                  int mode);

int setupShader();

void handleInput();

const GLuint WIDTH = 1000, HEIGHT = 1000;

const GLchar* vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texc;
uniform mat4 projection;
uniform mat4 model;
out vec2 texCoord;
void main()
{
   	gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);
	texCoord = texc;
})";

// Código fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar* fragmentShaderSource = R"(
#version 400
in vec2 texCoord;
uniform sampler2D texBuff;
out vec4 color;
void main()
{
	color = texture(texBuff,texCoord);
})";

Transformation trans;

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
    int vertexNum;
    GLuint VAO = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", vertexNum);

    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);
    glActiveTexture(GL_TEXTURE0);

    glm::mat4 projection = glm::mat4(1);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1,
                       GL_FALSE, glm::value_ptr(projection));

    glm::mat4 model = glm::mat4(1);
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    model =
        glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        handleInput();

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLineWidth(10);
        glPointSize(20);

        float angle = (GLfloat)glfwGetTime();

        model = glm::mat4(1);
        model = glm::rotate(model, trans.angleX, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, trans.angleY, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, trans.angleZ, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(trans.scale));
        model = glm::translate(model, glm::vec3(trans.posX, 0.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, trans.posY, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, trans.posZ));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(VAO);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glDrawArrays(GL_TRIANGLES, 0, vertexNum);

        // glDrawArrays(GL_POINTS, 0, vertexNum);

        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteTextures(1, &textureID);
    glfwTerminate();
    return 0;
}

bool keys[1024];  // Standard array to track key states

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
    if (keys[GLFW_KEY_X]) {
        trans.angleX += 0.1;
    }

    if (keys[GLFW_KEY_Y]) {
        trans.angleY += 0.1;
    }

    if (keys[GLFW_KEY_Z]) {
        trans.angleZ += 0.1;
    }

    if (keys[GLFW_KEY_W]) {
        trans.posZ += 0.1;
    }

    if (keys[GLFW_KEY_A]) {
        trans.posX -= 0.1;
    }

    if (keys[GLFW_KEY_S]) {
        trans.posZ -= 0.1;
    }

    if (keys[GLFW_KEY_D]) {
        trans.posX += 0.1;
    }

    if (keys[GLFW_KEY_I]) {
        trans.posY += 0.1;
    }

    if (keys[GLFW_KEY_J]) {
        trans.posY -= 0.1;
    }

    if (keys[GLFW_KEY_LEFT_BRACKET]) {
        trans.scale += 0.1;
    }

    if (keys[GLFW_KEY_RIGHT_BRACKET]) {
        trans.scale -= 0.1;
    }
}

int setupShader() {
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // Checando erros de compilação (exibição via log no terminal)
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // Checando erros de compilação (exibição via log no terminal)
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    // Linkando os shaders e criando o identificador do programa de shader
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // Checando por erros de linkagem
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
