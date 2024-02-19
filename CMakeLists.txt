cmake_minimum_required(VERSION 3.10)
project(learnOpenGL)

set(CMAKE_CXX_STANDARD 17)               # 设置C++标准为C++17

include_directories(/Users/xieyhccc/thirdpartylib/glfw-3.3.8/include)  # 添加glfw的头文件搜索路径
include_directories(/Users/xieyhccc/thirdpartylib/glad/include)       # 添加glad的头文件搜索路径
include_directories(/Users/xieyhccc/thirdpartylib/glm/include)        # 添加glm的头文件搜索路径
include_directories(/Users/xieyhccc/thirdpartylib/assimp/include)     # 添加assimp的头文件搜索路径

link_directories(/Users/xieyhccc/thirdpartylib/glfw-3.3.8/lib)        # 添加glfw库文件搜索路径                                               # 链接glfw3库
link_directories(/Users/xieyhccc/thirdpartylib/assimp/bin)            # 添加assimp库文件的搜索路径
find_library(COCOA_LIBRARY Cocoa)       # 查找Cocoa框架库
find_library(OPENGL_LIBRARY OpenGL)     # 查找OpenGL框架库
find_library(IOKIT_LIBRARY IOKit)       # 查找IOKit框架库

set(SOURCES
    src/glad.c
    src/openwindow.cpp
    src/shader.cpp)                 # 设置源文件路径

add_executable(openwindow ${SOURCES})   # 创建可执行文件目标
target_link_libraries(openwindow glfw3 assimp ${COCOA_LIBRARY} ${OPENGL_LIBRARY} ${IOKIT_LIBRARY})   # 链接框架库
