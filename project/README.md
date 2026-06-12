# Project: 3D Scene Viewer

Este é o projeto final para a Atividade Acadêmica de Computação Gráfica. Ele consiste em um visualizador de cenas 3D interativo com suporte a múltiplos objetos, materiais, iluminação dinâmica e um sistema de trajetórias (paths).

## 🚀 Setup

### Pré-requisitos
- **CMake**
- **Compilador C++** 
- **Bibliotecas do Sistema**:
  - OpenGL
  - GLFW3 (o CMake tentará encontrar ou baixar)
- **GLAD**: Siga as instruções no [README principal](../README.md) para gerar e colocar os arquivos `glad.c` e `glad.h` nos locais corretos.

### Compilação e Execução
1. No diretório raiz do repositório, crie uma pasta para a build:
   ```bash
   mkdir build
   cd build
   ```
2. Gere os arquivos de projeto com o CMake:
   ```bash
   cmake ..
   ```
3. Compile o projeto:
   ```bash
   make
   ```
4. Execute o visualizador:
   ```bash
   ./project/src/3d_viewer
   ```

---

## 📦 Assets

Os modelos 3D utilizados neste projeto foram obtidos de diversas fontes: 

- **Casa (House in the Beach)**: [Free3D - House in the Beach](https://free3d.com/3d-model/house-in-the-beach-659371.html)
- **Suzanne (Monkey)**: Fornecido pelo professor (modelo padrão do Blender).
- **Cube**: Fornecido pelo professor.

---

## 📚 Referências

### Documentação Técnica
- [OpenGL Documentation](https://www.opengl.org/documentation/)
- [Dear ImGui Wiki](https://github.com/ocornut/imgui/wiki)
- [tomlplusplus Documentation](https://marzer.github.io/tomlplusplus/)

### Tutoriais e Aprendizado
- [Learn OpenGL](https://learnopengl.com/): Principal referência para implementação de Shaders (Phong Lighting), Câmera e Gerenciamento de VBOs.
- [Catmull-Rom Splines Tutorial](https://www.mvps.org/directx/articles/catmull/): Base teórica para o sistema de animação de caminhos.
- Materiais disponíveis na comunidade do Moodle.

### Código
- [Snippets de Código](https://github.com/guilhermechagaskurtz/CGCCHibrido/tree/main/Code%20snippets)
- [Padrão de projeto e referências da cadeira anterior](https://github.com/otaviocap/processamento-grafico)

## Resultado

![Resultado do visualizador de cenas funcionando](/docs/3d_viewer.mp4)
