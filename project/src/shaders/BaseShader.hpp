#pragma once

#include <glad.h>

#include <string>

GLuint compileShaderFromSource(const std::string& shaderSource, GLenum shaderType);
GLuint createShaderProgram(const std::string& vertexPath,
                           const std::string& fragmentPath);
