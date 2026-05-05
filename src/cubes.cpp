
using namespace std;
#include <assert.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <iostream>
#include <string>

#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

class Cube {
  public:
    float angleX, angleY, angleZ;
    float posX, posY, posZ;
    float scale;

    Cube()
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
int setupGeometry();

void handleInput();

const GLuint WIDTH = 1000, HEIGHT = 1000;

const GLchar* vertexShaderSource =
    "#version 450\n"
    "layout (location = 0) in vec3 position;\n"
    "layout (location = 1) in vec3 color;\n"
    "uniform mat4 model;\n"
    "out vec4 finalColor;\n"
    "void main()\n"
    "{\n"
    "gl_Position = model * vec4(position, 1.0);\n"
    "finalColor = vec4(color, 1.0);\n"
    "}\0";

const GLchar* fragmentShaderSource =
    "#version 450\n"
    "in vec4 finalColor;\n"
    "out vec4 color;\n"
    "void main()\n"
    "{\n"
    "color = finalColor;\n"
    "}\n\0";

std::vector<Cube> cubes(10);
int selectedCube = 0;

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
    GLuint VAO = setupGeometry();

    glUseProgram(shaderID);

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

        for (int i = 0; i < cubes.size(); i++) {
            Cube c = cubes.at(i);

            model = glm::mat4(1);
            model = glm::rotate(model, c.angleX, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, c.angleY, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, c.angleZ, glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(c.scale));
            model = glm::translate(model, glm::vec3(c.posX, 0.0f, 0.0f));
            model = glm::translate(model, glm::vec3(0.0f, c.posY, 0.0f));
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, c.posZ));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            if (i == selectedCube) {
                glDrawArrays(GL_POINTS, 0, 18);
            }

            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }
    // Pede pra OpenGL desalocar os buffers
    glDeleteVertexArrays(1, &VAO);
    // Finaliza a execução da GLFW, limpando os recursos alocados por ela
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
    for (int i = 0; i <= 9; ++i) {
        if (keys[GLFW_KEY_0 + i]) {
            selectedCube = i;
        }
    }

    Cube& c = cubes.at(selectedCube);
    if (keys[GLFW_KEY_X]) {
        c.angleX += 0.1;
    }

    if (keys[GLFW_KEY_Y]) {
        c.angleY += 0.1;
    }

    if (keys[GLFW_KEY_Z]) {
        c.angleZ += 0.1;
    }

    if (keys[GLFW_KEY_W]) {
        c.posZ += 0.1;
    }

    if (keys[GLFW_KEY_A]) {
        c.posX -= 0.1;
    }

    if (keys[GLFW_KEY_S]) {
        c.posZ -= 0.1;
    }

    if (keys[GLFW_KEY_D]) {
        c.posX += 0.1;
    }

    if (keys[GLFW_KEY_I]) {
        c.posY += 0.1;
    }

    if (keys[GLFW_KEY_J]) {
        c.posY -= 0.1;
    }

    if (keys[GLFW_KEY_LEFT_BRACKET]) {
        c.scale += 0.1;
    }

    if (keys[GLFW_KEY_RIGHT_BRACKET]) {
        c.scale -= 0.1;
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

// Esta função está bastante harcoded - objetivo é criar os buffers que
// armazenam a geometria de um triângulo Apenas atributo coordenada nos vértices
// 1 VBO com as coordenadas, VAO com apenas 1 ponteiro para atributo
// A função retorna o identificador do VAO
int setupGeometry() {
    // Aqui setamos as coordenadas x, y e z do triângulo e as armazenamos de
    // forma sequencial, já visando mandar para o VBO (Vertex Buffer Objects)
    // Cada atributo do vértice (coordenada, cores, coordenadas de textura,
    // normal, etc) Pode ser arazenado em um VBO único ou em VBOs separados

    // clang-format off
 GLfloat vertices[] = {
    // BACK FACE (z = -0.5)
    -0.5, -0.5, -0.5,  1.0, 0.0, 0.0,
     0.5, -0.5, -0.5,  1.0, 0.0, 0.0,
     0.5,  0.5, -0.5,  1.0, 0.0, 0.0,
     0.5,  0.5, -0.5,  1.0, 0.0, 0.0,
    -0.5,  0.5, -0.5,  1.0, 0.0, 0.0,
    -0.5, -0.5, -0.5,  1.0, 0.0, 0.0,

    // FRONT FACE (z = 0.5)
    -0.5, -0.5,  0.5,  0.0, 1.0, 0.0,
     0.5, -0.5,  0.5,  0.0, 1.0, 0.0,
     0.5,  0.5,  0.5,  0.0, 1.0, 0.0,
     0.5,  0.5,  0.5,  0.0, 1.0, 0.0,
    -0.5,  0.5,  0.5,  0.0, 1.0, 0.0,
    -0.5, -0.5,  0.5,  0.0, 1.0, 0.0,

    // LEFT FACE (x = -0.5)
    -0.5,  0.5,  0.5,  0.0, 0.0, 1.0,
    -0.5,  0.5, -0.5,  0.0, 0.0, 1.0,
    -0.5, -0.5, -0.5,  0.0, 0.0, 1.0,
    -0.5, -0.5, -0.5,  0.0, 0.0, 1.0,
    -0.5, -0.5,  0.5,  0.0, 0.0, 1.0,
    -0.5,  0.5,  0.5,  0.0, 0.0, 1.0,

    // RIGHT FACE (x = 0.5)
     0.5,  0.5,  0.5,  1.0, 1.0, 0.0,
     0.5,  0.5, -0.5,  1.0, 1.0, 0.0,
     0.5, -0.5, -0.5,  1.0, 1.0, 0.0,
     0.5, -0.5, -0.5,  1.0, 1.0, 0.0,
     0.5, -0.5,  0.5,  1.0, 1.0, 0.0,
     0.5,  0.5,  0.5,  1.0, 1.0, 0.0,

    // BOTTOM FACE (y = -0.5)
    -0.5, -0.5, -0.5,  1.0, 0.0, 1.0,
     0.5, -0.5, -0.5,  1.0, 0.0, 1.0,
     0.5, -0.5,  0.5,  1.0, 0.0, 1.0,
     0.5, -0.5,  0.5,  1.0, 0.0, 1.0,
    -0.5, -0.5,  0.5,  1.0, 0.0, 1.0,
    -0.5, -0.5, -0.5,  1.0, 0.0, 1.0,

    // TOP FACE (y = 0.5)
    -0.5,  0.5, -0.5,  0.0, 1.0, 1.0,
     0.5,  0.5, -0.5,  0.0, 1.0, 1.0,
     0.5,  0.5,  0.5,  0.0, 1.0, 1.0,
     0.5,  0.5,  0.5,  0.0, 1.0, 1.0,
    -0.5,  0.5,  0.5,  0.0, 1.0, 1.0,
    -0.5,  0.5, -0.5,  0.0, 1.0, 1.0,
};
    // clang-format on

    GLuint VBO, VAO;

    // Geração do identificador do VBO
    glGenBuffers(1, &VBO);

    // Faz a conexão (vincula) do buffer como um buffer de array
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Envia os dados do array de floats para o buffer da OpenGl
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Geração do identificador do VAO (Vertex Array Object)
    glGenVertexArrays(1, &VAO);

    // Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s)
    // buffer(s) de vértices e os ponteiros para os atributos
    glBindVertexArray(VAO);

    // Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para
    // o atributo), indicando:
    //  Localização no shader * (a localização dos atributos devem ser
    //  correspondentes no layout especificado no vertex shader) Numero de
    //  valores que o atributo tem (por ex, 3 coordenadas xyz) Tipo do dado Se
    //  está normalizado (entre zero e um) Tamanho em bytes Deslocamento a
    //  partir do byte zero

    // Atributo posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat),
                          (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Atributo cor (r, g, b)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat),
                          (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Observe que isso é permitido, a chamada para glVertexAttribPointer
    // registrou o VBO como o objeto de buffer de vértice atualmente vinculado -
    // para que depois possamos desvincular com segurança
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array
    // para evitar bugs medonhos)
    glBindVertexArray(0);

    return VAO;
}