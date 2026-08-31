cmake_minimum_required(VERSION 3.20)

project(ShaderLab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTORCC ON)

find_package(
    Qt6
    REQUIRED
    COMPONENTS
        Widgets
        OpenGLWidgets
)

qt_standard_project_setup()

add_executable(
    shaderlab

    src/main.cpp
    src/main_window.cpp
    src/render_widget.cpp
    src/param_form.cpp
    src/project_tree_panel.cpp
    src/shader_file_utils.cpp
    src/theme.cpp

    src/main_window.h
    src/render_widget.h
    src/param.h
    src/param_form.h
    src/project_tree_panel.h
    src/theme.h

    resources/resources.qrc
)

target_include_directories(
    shaderlab
    PRIVATE
        src
)

target_link_libraries(
    shaderlab
    PRIVATE
        Qt6::Widgets
        Qt6::OpenGLWidgets
)

add_custom_command(
    TARGET shaderlab
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND}
        -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources/shaders/gallery
        $<TARGET_FILE_DIR:shaderlab>/shaders/gallery
)
