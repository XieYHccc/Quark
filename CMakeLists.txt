cmake_minimum_required(VERSION 3.10)
project(Quark)

set(CMAKE_CXX_STANDARD 20)              

message("-- [Quark] CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

# set output path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin )
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(ASSETS_PATH ${PROJECT_SOURCE_DIR}/Assets)
# find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)

################################################################################
# Third Party                                                                  #
################################################################################
set(EXTERNAL_PATH ${CMAKE_SOURCE_DIR}/External) # path to dependencies

set(GLFW_SOURCE_DIR ${EXTERNAL_PATH}/glfw)
set(GLAD_SOURCE_DIR ${EXTERNAL_PATH}/glad-4.6)
set(STB_IMAGE_SOURCE_DIR ${EXTERNAL_PATH}/stb_image-2.28)
set(STB_IMAGE_WRITE_SOURCE_DIR ${EXTERNAL_PATH}/stb_image_write-1.16)
set(TINY_OBJ_LOADER_SOURCE_DIR ${EXTERNAL_PATH}/tinyobjloader)
set(IMGUI_SOURCE_DIR ${EXTERNAL_PATH}/imgui)
set(FASTGLTF_SOURCE_DIR ${EXTERNAL_PATH}/fastgltf)
set(LZ4_SOURCE_DIR ${EXTERNAL_PATH}/lz4)
set(NLOHMANN_JSON_SOURCE_DIR ${EXTERNAL_PATH}/nlohmann-json)
set(GLM_SOURCE_DIR ${EXTERNAL_PATH}/glm)
set(VMA_SOURCE_DIR ${EXTERNAL_PATH}/vma)
set(SPIRV_REFLECT_DIR ${EXTERNAL_PATH}/spirv_reflect)

# Building STB, GLAD, and TINY_OBJ_LOADER lib
add_subdirectory(${STB_IMAGE_SOURCE_DIR})
add_subdirectory(${STB_IMAGE_WRITE_SOURCE_DIR})
# add_subdirectory(${GLAD_SOURCE_DIR})
add_subdirectory(${TINY_OBJ_LOADER_SOURCE_DIR})
add_subdirectory(${IMGUI_SOURCE_DIR})
add_subdirectory(${FASTGLTF_SOURCE_DIR})
add_subdirectory(${LZ4_SOURCE_DIR})
add_subdirectory(${NLOHMANN_JSON_SOURCE_DIR})
add_subdirectory(${SPIRV_REFLECT_DIR})

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

# Build GLM Interface lib
add_library(glm INTERFACE) 
target_include_directories(glm INTERFACE ${GLM_SOURCE_DIR})

#Build vma Interface lib
add_library(vma INTERFACE) 
target_include_directories(vma INTERFACE ${VMA_SOURCE_DIR}/include)

# Find glslang
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

################################################################################
# Configuration                                                                #
################################################################################
if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    add_definitions(-DQK_DEBUG_BUILD)
elseif(${CMAKE_BUILD_TYPE} MATCHES Release)
    add_definitions(-DQK_RELEASE_BUILD)
else()
    message( FATAL_ERROR "Unknown build type!" )
endif()

################################################################################
# Quark                                                                        #
################################################################################


add_subdirectory(./Engine)
add_subdirectory(./SandBox)
add_subdirectory(./VulkanApp)
add_subdirectory(./TestBed)

################################################################################
# Shader Compilation                                                           #
################################################################################
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${ASSETS_PATH}/Shaders/*.frag"
    "${ASSETS_PATH}/Shaders/*.vert"
    "${ASSETS_PATH}/Shaders/*.comp"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${ASSETS_PATH}/Shaders/Spirv/${FILE_NAME}.spv")
  message(STATUS ${GLSL})
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
)
