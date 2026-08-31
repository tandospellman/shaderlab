ShaderLab drop-in update

ADD:
  src/theme.h
  src/theme.cpp
  resources/shaders/gallery/*

REPLACE:
  src/main.cpp
  src/main_window.cpp
  src/param_form.cpp
  src/project_tree_panel.cpp
  src/project_tree_panel.h
  CMakeLists.txt

KEEP your existing:
  src/main_window.h
  src/param_form.h
  src/render_widget.cpp
  src/render_widget.h
  src/param.h
  src/shader_file_utils.cpp
  src/shader_file_utils.h
  resources/resources.qrc

Build:
  rm -rf build
  cmake -S . -B build -G Ninja
  cmake --build build
  ./build/shaderlab
