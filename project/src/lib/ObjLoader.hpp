#pragma once

#include <glad.h>

#include <string>
#include <vector>
#include <map>

#include "GameObject.hpp"
#include "Material.hpp"

std::string getDirectory(const std::string& filePath);
std::map<std::string, std::pair<Material, std::string>> loadAllMaterialsFromMTL(const std::string& mtlPath);
void loadMultiMaterialOBJ(const std::string& filePath, std::vector<MeshPart>& outMeshes);
