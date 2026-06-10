#include "ObjLoader.hpp"

#include <algorithm>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <vector>

#include "Texture.hpp"
#include "VboHelper.hpp"

constexpr int vertexStride = 8;

std::string getDirectory(const std::string& filePath) {
    const size_t found = filePath.find_last_of("/\\");
    if (found == std::string::npos) {
        return "";
    }
    return filePath.substr(0, found + 1);
}

Material loadMaterialFromMTL(const std::string& mtlPath,
                             std::string& textureFile) {
    Material material;
    textureFile.clear();

    std::ifstream file(mtlPath.c_str());
    if (!file.is_open()) {
        std::cerr << "Erro ao tentar ler o arquivo " << mtlPath << std::endl;
        return material;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string word;
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

GLuint loadSimpleOBJ(const std::string& filePath, int& nVertices,
                     GLuint& textureId, Material& material) {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    std::string mtlFile;
    const std::string objDirectory = getDirectory(filePath);

    std::ifstream file(filePath.c_str());
    if (!file.is_open()) {
        std::cerr << "Erro ao tentar ler o arquivo " << filePath << std::endl;
        return 0;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string word;
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
                std::istringstream ssIndex(word);
                std::string index;

                if (std::getline(ssIndex, index, '/')) {
                    vi = !index.empty() ? std::stoi(index) - 1 : 0;
                }
                if (std::getline(ssIndex, index, '/')) {
                    ti = !index.empty() ? std::stoi(index) - 1 : -1;
                }
                if (std::getline(ssIndex, index)) {
                    ni = !index.empty() ? std::stoi(index) - 1 : -1;
                }

                const glm::vec2 texCoord =
                    ti >= 0 && ti < static_cast<int>(texCoords.size())
                        ? texCoords[ti]
                        : glm::vec2(0.0f, 0.0f);
                const glm::vec3 normal =
                    ni >= 0 && ni < static_cast<int>(normals.size())
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
        std::string textureFile;
        material = loadMaterialFromMTL(objDirectory + mtlFile, textureFile);
        if (!textureFile.empty()) {
            textureId = loadTexture(objDirectory + textureFile);
        }
    }

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    createVBOAndBind(VAO, vBuffer.data(), static_cast<int>(vBuffer.size()));

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          vertexStride * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          vertexStride * sizeof(GLfloat),
                          (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                          vertexStride * sizeof(GLfloat),
                          (GLvoid*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = static_cast<int>(vBuffer.size() / vertexStride);
    return VAO;
}
