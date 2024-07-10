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
add_subdirectory(External)

set(EXTERNAL_PATH ${CMAKE_SOURCE_DIR}/External) # path to dependencies

set(GLAD_SOURCE_DIR ${EXTERNAL_PATH}/glad-4.6)
set(FASTGLTF_SOURCE_DIR ${EXTERNAL_PATH}/fastgltf)

# add_subdirectory(${GLAD_SOURCE_DIR})
add_subdirectory(${FASTGLTF_SOURCE_DIR})

# Find glslang
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

################################################################################
# Configuration                                                                #
################################################################################
if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    add_definitions(-DQK_DEBUG_BUILD)
elseif(${CMAKE_BUILD_TYPE} MATCHES Release)
    add_definitions(-DQK_RELEASE_BUILD)
elseif(${CMAKE_BUILD_TYPE} MATCHES RelWithDebInfo)
    add_definitions(-DQK_DEBUG_BUILD)
else()
    message( FATAL_ERROR "Unknown build type!" )
endif()

################################################################################
# Quark                                                                        #
################################################################################
add_subdirectory(./Engine)
#add_subdirectory(./SandBox)
add_subdirectory(./TestBed)
add_subdirectory(./Examples)

################################################################################
# Shaders Compilation                                                           #
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
