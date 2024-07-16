set(CMAKE_CXX_STANDARD 20)

FILE(GLOB_RECURSE EngineSources ${CMAKE_SOURCE_DIR}/src/Engine "*.cpp" "*.c" "*.h")

add_library(Engine ${EngineSources})

set(ENGINE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ENGINE_INCLUDE_DIRS
${ENGINE_SOURCE_DIR}
)

target_compile_options(Engine PRIVATE -Wno-nullability-completeness)
# target_compile_definitions(Engine PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_include_directories(Engine PUBLIC ${ENGINE_INCLUDE_DIRS} )
target_link_libraries(Engine glm tinygltf tiny_obj_loader stb_image glfw Vulkan::Vulkan vma imgui spirv_reflect spdlog::spdlog)

add_dependencies(Engine Shaders)

# pch header file path
set(ENGINE_PRECOMPILED_HEADERS
    ${ENGINE_SOURCE_DIR}/pch.h
)
target_precompile_headers(Engine PRIVATE ${ENGINE_PRECOMPILED_HEADERS})

# copy shader to binary file directory
set(SPIRV_SOURCE_DIR "${ASSETS_PATH}")
set(SPIRV_DESTINATION_DIR "${EXECUTABLE_OUTPUT_PATH}/Assets")
add_custom_command(
    TARGET Engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${SPIRV_SOURCE_DIR} ${SPIRV_DESTINATION_DIR}
)
