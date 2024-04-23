cmake_minimum_required(VERSION 3.10)
project(XEngine)

set(CMAKE_CXX_STANDARD 20)              

# set output path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin )
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)

# Set 3rd party source directory
set(GLFW_SOURCE_DIR "external/glfw")
set(GLAD_SOURCE_DIR "external/glad-4.6")
set(STB_IMAGE_SOURCE_DIR "external/stb_image-2.28")
set(STB_IMAGE_WRITE_SOURCE_DIR "external/stb_image_write-1.16")
set(TINY_OBJ_LOADER_SOURCE_DIR "external/tinyobjloader")
set(IMGUI_SOURCE_DIR "external/imgui")
set(FASTGLTF_SOURCE_DIR "external/fastgltf")

# Building STB, GLAD, and TINY_OBJ_LOADER lib
add_subdirectory(${STB_IMAGE_SOURCE_DIR} ${STB_IMAGE_WRITE_SOURCE_DIR})
add_subdirectory(${GLAD_SOURCE_DIR})
add_subdirectory(${TINY_OBJ_LOADER_SOURCE_DIR})
add_subdirectory(${IMGUI_SOURCE_DIR})
add_subdirectory(${FASTGLTF_SOURCE_DIR})


# Building only the Assimp lib
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
# set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
# set(ASSIMP_INSTALL OFF CACHE BOOL "")
# add_subdirectory(${ASSIMP_SOURCE_DIR})

# Building only the GLFW lib
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")
add_subdirectory(${GLFW_SOURCE_DIR})

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

add_subdirectory(./src)