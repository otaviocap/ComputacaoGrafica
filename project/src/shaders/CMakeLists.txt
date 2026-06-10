add_library(3d_viewer_shaders STATIC
    BaseShader.cpp
)

target_include_directories(3d_viewer_shaders PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include/glad
)
