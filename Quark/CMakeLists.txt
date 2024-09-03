set(CMAKE_CXX_STANDARD 20)

set(QUARK_SOURCE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(QUARK_INCLUDE_DIRS ${QUARK_SOURCE_ROOT_DIR})

FILE(GLOB_RECURSE QUARK_SOURCE_FILES ${QUARK_SOURCE_ROOT_DIR} "*.cpp" "*.c" "*.h")

# Group source files
source_group(TREE ${QUARK_SOURCE_ROOT_DIR} FILES ${QUARK_SOURCE_FILES})

add_library(quark ${QUARK_SOURCE_FILES})

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

target_include_directories(quark PUBLIC ${CMAKE_SOURCE_DIR} )

target_link_libraries(quark glm tinygltf tiny_obj_loader stb_image glfw Vulkan::Vulkan 
    vma imgui spirv_reflect spdlog::spdlog ktx basisu yaml-cpp::yaml-cpp nfd glslang::glslang
    glslang-default-resource-limits
    )

target_precompile_headers(quark PRIVATE ${QUARK_SOURCE_ROOT_DIR}/qkpch.h)

add_dependencies(quark Shaders)

# copy shader to binary file directory
# set(SPIRV_SOURCE_DIR "${ASSETS_PATH}")
# set(SPIRV_DESTINATION_DIR "${EXECUTABLE_OUTPUT_PATH}/Assets")
# add_custom_command(
#     TARGET quark POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory ${SPIRV_SOURCE_DIR} ${SPIRV_DESTINATION_DIR}
# )
