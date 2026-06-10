#pragma once

#include <glad.h>

#include <string>

#include "GameObject.hpp"
#include "Material.hpp"

std::string getDirectory(const std::string& filePath);
Material loadMaterialFromMTL(const std::string& mtlPath, std::string& textureFile);
GLuint loadSimpleOBJ(const std::string& filePath, int& nVertices,
                     GLuint& textureId, Material& material);
