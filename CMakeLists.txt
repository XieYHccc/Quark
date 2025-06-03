cmake_minimum_required(VERSION 3.19)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
project(Quark VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)      
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

set(QUARK_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_INSTALL_PREFIX "${QUARK_ROOT_DIR}/bin")
set(BINARY_ROOT_DIR "${CMAKE_INSTALL_PREFIX}/")
set(BUILTIN_REOURCES_DIR ${PROJECT_SOURCE_DIR}/BuiltInResources)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options("/MP")
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT QuarkEditor)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)
find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

add_subdirectory(ThirdParty)
add_subdirectory(Quark)
# add_subdirectory(./TestBed)
# add_subdirectory(./Examples)
add_subdirectory(Editor)

function(add_quark_application TARGET_NAME)
	add_executable(${TARGET_NAME} ${ARGN})
    target_link_libraries(${TARGET_NAME} quark)
endfunction()

function(add_quark_test TARGET_NAME)
	add_quark_application(${TARGET_NAME} ${ARGN})
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests")
endfunction()

add_subdirectory(Tests)


file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${BUILTIN_REOURCES_DIR}/Shaders/*.frag"
    "${BUILTIN_REOURCES_DIR}/Shaders/*.vert"
    "${BUILTIN_REOURCES_DIR}/Shaders/*.comp"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  # message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${BUILTIN_REOURCES_DIR}/Shaders/Spirv/${FILE_NAME}.spv")
  # message(STATUS ${GLSL})
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})

# set_source_files_properties(${GLSL_SOURCE_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
# add_custom_target(GLSL_Files SOURCES ${GLSL_SOURCE_FILES})

