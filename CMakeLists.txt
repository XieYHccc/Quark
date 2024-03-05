cmake_minimum_required(VERSION 3.10)
project(XEngine)

set(CMAKE_CXX_STANDARD 17)              

# set output path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build )

# set source directory
set(GLFW_SOURCE_DIR "external/glfw")
set(GLAD_SOURCE_DIR "external/glad-4.6")
set(ASSIMP_SOURCE_DIR "external/assimp")
set(STB_IMAGE_SOURCE_DIR "external/stb_image-2.28")
set(STB_IMAGE_WRITE_SOURCE_DIR "external/stb_image_write-1.16")
set(RTTR_SOURCE_DIR "external/rttr")
set(TINY_OBJ_LOADER_SOURCE_DIR "external/tinyobjloader")

# Building STB, GLAD, and TINY_OBJ_LOADER lib
add_subdirectory(${STB_IMAGE_SOURCE_DIR} ${STB_IMAGE_WRITE_SOURCE_DIR})
add_subdirectory(${GLAD_SOURCE_DIR})
add_subdirectory(${TINY_OBJ_LOADER_SOURCE_DIR})

# Building only the Assimp lib
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_INSTALL OFF CACHE BOOL "")
# add_subdirectory(${ASSIMP_SOURCE_DIR})

# Building only the GLFW lib
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")
add_subdirectory(${GLFW_SOURCE_DIR})

# Building RTTR lib
#set(BUILD_RTTR_DYNAMIC OFF CACHE BOOL "")
#set(BUILD_STATIC ON CACHE BOOL "")
#set(BUILD_UNIT_TESTS  OFF CACHE BOOL "")
#set(BUILD_EXAMPLES  OFF CACHE BOOL "")
#set(BUILD_DOCUMENTATION  OFF CACHE BOOL "")
#set(BUILD_INSTALLER  OFF CACHE BOOL "")
#set(BUILD_PACKAGE  OFF CACHE BOOL "")
#add_subdirectory(${RTTR_SOURCE_DIR})

# Add include directories
include_directories(${PROJECT_SOURCE_DIR}/external/glm)
include_directories(${PROJECT_SOURCE_DIR}/external/stb_image-2.28)
include_directories(${PROJECT_SOURCE_DIR}/external/stb_image_write-1.16)
include_directories(${PROJECT_SOURCE_DIR}/external/tinyobjloader)
include_directories("${GLAD_SOURCE_DIR}/include")

find_package(OpenGL REQUIRED)

add_subdirectory(./src)