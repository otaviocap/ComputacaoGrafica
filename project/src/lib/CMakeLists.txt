add_library(3d_viewer_lib STATIC
    Camera.cpp
    Texture.cpp
    VboHelper.cpp
    ObjLoader.cpp
    PathAnimator.cpp
    DebugRenderer.cpp
)

target_include_directories(3d_viewer_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include/glad
    ${glm_SOURCE_DIR}
    ${stb_image_SOURCE_DIR}
)

target_link_libraries(3d_viewer_lib PRIVATE glfw ${OPENGL_LIBS})
