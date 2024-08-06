set(CMAKE_CXX_STANDARD 20)

FILE(GLOB_RECURSE EngineSources ${CMAKE_SOURCE_DIR}/src/Engine "*.cpp" "*.c" "*.h")

add_library(quark ${EngineSources})

set(ENGINE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ENGINE_INCLUDE_DIRS ${ENGINE_SOURCE_DIR})

if(MSVC)

else()
    #for clang and gcc
    target_compile_options(quark PRIVATE -Wno-nullability-completeness)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_definitions(quark PUBLIC QK_DEBUG_BUILD)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(quark PUBLIC QK_RELEASE_BUILD)
else()
    message( FATAL_ERROR "Unknown build type!" )
endif()

target_include_directories(quark PUBLIC ${ENGINE_INCLUDE_DIRS} )
target_link_libraries(quark glm tinygltf tiny_obj_loader stb_image glfw Vulkan::Vulkan vma imgui spirv_reflect spdlog::spdlog ktx basisu)

add_dependencies(quark Shaders)

# pch header file path
set(ENGINE_PRECOMPILED_HEADERS
    ${ENGINE_SOURCE_DIR}/qkpch.h
)
target_precompile_headers(quark PRIVATE ${ENGINE_PRECOMPILED_HEADERS})

# copy shader to binary file directory
set(SPIRV_SOURCE_DIR "${ASSETS_PATH}")
set(SPIRV_DESTINATION_DIR "${EXECUTABLE_OUTPUT_PATH}/Assets")
add_custom_command(
    TARGET quark POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SPIRV_SOURCE_DIR} ${SPIRV_DESTINATION_DIR}
)
