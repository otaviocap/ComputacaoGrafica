cmake_minimum_required(VERSION 3.10)
project(OPENGLEXERCISES)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4  # Define a versão desejada da GLFW
)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG master  # Define a versão desejada da GLM
)

FetchContent_Declare(
  dear_im_gui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
)
FetchContent_MakeAvailable(glfw glm dear_im_gui)

FetchContent_Declare(
        stb_image
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
)

FetchContent_MakeAvailable(stb_image)

# Adiciona as pastas de cabeçalhos
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/include/glad)
include_directories(${glm_SOURCE_DIR})


set(EXERCISES
    cubes
    obj
    vivencial1
    phong
    fps
    3lights
    paths
)

add_compile_options(-Wno-pragmas)

if(WIN32)
    set(OPENGL_LIBS opengl32)
elseif(APPLE)
    find_library(OpenGL_LIBRARY OpenGL)
    set(OPENGL_LIBS ${OpenGL_LIBRARY})
else()
    find_package(OpenGL REQUIRED)
    set(OPENGL_LIBS ${OPENGL_gl_LIBRARY})
endif()

set(GLAD_C_FILE "${CMAKE_SOURCE_DIR}/common/glad.c")

if (NOT EXISTS ${GLAD_C_FILE})
    message(FATAL_ERROR "Arquivo glad.c não encontrado! Baixe a GLAD manualmente em https://glad.dav1d.de/ e coloque glad.h em include/glad/ e glad.c em common/")
endif()

foreach(EXERCISE ${EXERCISES})
    get_filename_component(EXE_NAME ${EXERCISE} NAME)
    add_executable(${EXE_NAME} exercises/${EXERCISE}.cpp ${GLAD_C_FILE})
    target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include/glad ${glm_SOURCE_DIR} ${stb_image_SOURCE_DIR})
    target_link_libraries(${EXE_NAME} glfw ${OPENGL_LIBS})
endforeach()

add_subdirectory(project/src)
