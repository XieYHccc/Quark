cmake_minimum_required(VERSION 3.10)
project(Quark)

set(CMAKE_CXX_STANDARD 20)              

message("-- [Quark] CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

# set output path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin )
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(BUILTIN_REOURCES_DIR ${PROJECT_SOURCE_DIR}/BuiltInResources)

# find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)

################################################################################
# Third Party                                                                  #
################################################################################
add_subdirectory(ThirdParty)

set(EXTERNAL_PATH ${CMAKE_SOURCE_DIR}/ThirdParty) # path to dependencies

set(GLAD_SOURCE_DIR ${EXTERNAL_PATH}/glad-4.6)

# add_subdirectory(${GLAD_SOURCE_DIR})

# Find glslang
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

################################################################################
# Configuration                                                                #
################################################################################

if (MSVC)
    add_compile_options(/MP)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

################################################################################
# Quark                                                                        #
################################################################################
add_subdirectory(./Quark)
#add_subdirectory(./TestBed)
#add_subdirectory(./Examples)
add_subdirectory(./Editor)
add_subdirectory(./Tests)

################################################################################
# Shaders Compilation                                                           #
################################################################################
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${BUILTIN_REOURCES_DIR}/Shaders/*.frag"
    "${BUILTIN_REOURCES_DIR}/Shaders/*.vert"
    "${BUILTIN_REOURCES_DIR}/Shaders/*.comp"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${BUILTIN_REOURCES_DIR}/Shaders/Spirv/${FILE_NAME}.spv")
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

# copy Assets to build directory
file(COPY ${BUILTIN_REOURCES_DIR} DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/Editor/imgui.ini DESTINATION ${EXECUTABLE_OUTPUT_PATH})
