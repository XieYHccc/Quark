cmake_minimum_required(VERSION 3.10)
project(learnOpenGL)

set(CMAKE_CXX_STANDARD 20)              

set(GLFW_SOURCE_DIR "external/glfw")
set(GLAD_SOURCE_DIR "external/glad-4.6")
set(ASSIMP_SOURCE_DIR "external/assimp")
set(STB_IMAGE_SOURCE_DIR "external/stb_image-2.28")
set(STB_IMAGE_WRITE_SOURCE_DIR "external/stb_image_write-1.16")

# Building STB and GLAD lib
add_subdirectory(${STB_IMAGE_SOURCE_DIR} ${STB_IMAGE_WRITE_SOURCE_DIR})
add_subdirectory(${GLAD_SOURCE_DIR})

# Building only the Assimp lib
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_INSTALL OFF CACHE BOOL "")
add_subdirectory(${ASSIMP_SOURCE_DIR})

# Building only the GLFW lib
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")
add_subdirectory(${GLFW_SOURCE_DIR})


# Add include directories
include_directories(${PROJECT_SOURCE_DIR}/external/GLM)
include_directories(${PROJECT_SOURCE_DIR}/external/stb_image-2.28)
include_directories("${PROJECT_SOURCE_DIR}/external/XMath/include")
include_directories(${PROJECT_SOURCE_DIR}/external/stb_image_write-1.16)
include_directories("${GLAD_SOURCE_DIR}/include")

find_package(OpenGL REQUIRED)

add_subdirectory(./src)