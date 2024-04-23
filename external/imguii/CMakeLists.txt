add_library(imgui STATIC)

target_include_directories(imgui PUBLIC imgui)

target_sources(imgui PRIVATE 
  imgui.h
  imgui_impl_glfw.cpp
  imgui_impl_vulkan.cpp
  imgui.cpp
  imgui_demo.cpp
  imgui_draw.cpp
  imgui_widgets.cpp
  imgui_tables.cpp
  )

target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)