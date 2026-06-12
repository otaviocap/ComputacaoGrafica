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

struct VertexIndices {
    int v, t, n;
};

std::string getDirectory(const std::string& filePath) {
    const size_t found = filePath.find_last_of("/\\");
    if (found == std::string::npos) {
        return "";
    }
    return filePath.substr(0, found + 1);
}

Material loadMaterialFromMTL(const std::string& mtlPath,
                             const std::string& targetMaterial,
                             std::string& textureFile) {
    Material material;
    textureFile.clear();

    std::ifstream file(mtlPath.c_str());
    if (!file.is_open()) {
        std::cerr << "Erro ao tentar ler o arquivo " << mtlPath << std::endl;
        return material;
    }

    std::string line;
    bool readingTarget = targetMaterial.empty();
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string word;
        ss >> word;

        if (word == "newmtl") {
            std::string name;
            ss >> name;
            if (!targetMaterial.empty()) {
                readingTarget = (name == targetMaterial);
            } else {
                readingTarget = true;
            }
        }

        if (!readingTarget) continue;

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

void addVertexToBuffer(const VertexIndices& idx,
                       const std::vector<glm::vec3>& vertices,
                       const std::vector<glm::vec2>& texCoords,
                       const std::vector<glm::vec3>& normals,
                       std::vector<GLfloat>& vBuffer) {
    if (idx.v < 0 || idx.v >= static_cast<int>(vertices.size())) return;

    const glm::vec3& v = vertices[idx.v];
    const glm::vec2& t =
        (idx.t >= 0 && idx.t < static_cast<int>(texCoords.size()))
            ? texCoords[idx.t]
            : glm::vec2(0.0f);
    const glm::vec3& n =
        (idx.n >= 0 && idx.n < static_cast<int>(normals.size()))
            ? normals[idx.n]
            : glm::vec3(0.0f, 0.0f, 1.0f);

    vBuffer.push_back(v.x);
    vBuffer.push_back(v.y);
    vBuffer.push_back(v.z);
    vBuffer.push_back(t.s);
    vBuffer.push_back(1.0f - t.t);
    vBuffer.push_back(n.x);
    vBuffer.push_back(n.y);
    vBuffer.push_back(n.z);
}

GLuint loadSimpleOBJ(const std::string& filePath, int& nVertices,
                     GLuint& textureId, Material& material) {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    std::string mtlFile;
    std::string activeMaterial;
    const std::string objDirectory = getDirectory(filePath);

    std::ifstream file(filePath.c_str());
    if (!file.is_open()) {
        std::cerr << "Erro ao tentar ler o arquivo " << filePath << std::endl;
        return 0;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string word;
        if (!(ss >> word)) continue;

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
            std::vector<VertexIndices> face;
            while (ss >> word) {
                VertexIndices idx = {-1, -1, -1};
                size_t firstSlash = word.find('/');
                size_t lastSlash = word.rfind('/');

                idx.v = std::stoi(word.substr(0, firstSlash)) - 1;
                if (firstSlash != std::string::npos) {
                    std::string texIdx = word.substr(
                        firstSlash + 1,
                        (lastSlash == firstSlash ? std::string::npos
                                                 : lastSlash - firstSlash - 1));
                    if (!texIdx.empty()) idx.t = std::stoi(texIdx) - 1;

                    if (lastSlash != firstSlash) {
                        std::string normIdx = word.substr(lastSlash + 1);
                        if (!normIdx.empty()) idx.n = std::stoi(normIdx) - 1;
                    }
                }
                face.push_back(idx);
            }

            // Triangle Fan Triangulation
            for (size_t i = 1; i < face.size() - 1; ++i) {
                addVertexToBuffer(face[0], vertices, texCoords, normals,
                                  vBuffer);
                addVertexToBuffer(face[i], vertices, texCoords, normals,
                                  vBuffer);
                addVertexToBuffer(face[i + 1], vertices, texCoords, normals,
                                  vBuffer);
            }
        } else if (word == "mtllib") {
            ss >> mtlFile;
        } else if (word == "usemtl") {
            if (activeMaterial.empty()) {
                ss >> activeMaterial;
            }
        }
    }

    file.close();

    if (!mtlFile.empty()) {
        std::string textureFile;
        material = loadMaterialFromMTL(objDirectory + mtlFile, activeMaterial,
                                       textureFile);
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
