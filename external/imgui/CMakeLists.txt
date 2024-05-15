add_library(imgui STATIC
  imgui_impl_glfw.cpp
  imgui_impl_vulkan.cpp
  imgui.cpp
  imgui_demo.cpp
  imgui_draw.cpp
  imgui_widgets.cpp
  imgui_tables.cpp
)

target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)