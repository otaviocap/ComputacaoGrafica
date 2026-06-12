
add_executable(3d_viewer main.cpp ${GLAD_C_FILE})

add_subdirectory(lib)
add_subdirectory(shaders)

target_compile_definitions(3d_viewer PRIVATE
    VIEWER_ASSETS_DIR=\"${CMAKE_SOURCE_DIR}/assets\"
    VIEWER_SHADER_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/../shaders\"
)

add_library(imgui
	${dear_im_gui_SOURCE_DIR}/imgui.cpp
	${dear_im_gui_SOURCE_DIR}/imgui_draw.cpp
	${dear_im_gui_SOURCE_DIR}/imgui_tables.cpp
	${dear_im_gui_SOURCE_DIR}/imgui_widgets.cpp
	${dear_im_gui_SOURCE_DIR}/imgui_demo.cpp
    ${dear_im_gui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${dear_im_gui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_link_libraries(imgui PRIVATE glfw)

target_include_directories(imgui PUBLIC ${dear_im_gui_SOURCE_DIR})

target_include_directories(3d_viewer PRIVATE
    ${CMAKE_SOURCE_DIR}/include/glad
    ${glm_SOURCE_DIR}
    ${stb_image_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
)

target_link_libraries(3d_viewer PRIVATE
    3d_viewer_lib
    3d_viewer_shaders
    glfw
    imgui
    tomlplusplus::tomlplusplus
    ${OPENGL_LIBS}
)
