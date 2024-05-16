set(CMAKE_CXX_STANDARD 20)

FILE(GLOB_RECURSE EngineSources ${CMAKE_SOURCE_DIR}/src/Engine "*.cpp" "*.c" "*.h")

add_library(Engine ${EngineSources})

set(ENGINE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ENGINE_INCLUDE_DIRS
${ENGINE_SOURCE_DIR}
${EXTERNAL_PATH}/spdlog/include
)

target_compile_options(Engine PRIVATE -Wno-nullability-completeness)
target_compile_definitions(Engine PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_include_directories(Engine PUBLIC ${ENGINE_INCLUDE_DIRS} )
target_link_libraries(Engine glm tiny_obj_loader stb_image glfw Vulkan::Vulkan vma imgui fastgltf::fastgltf)


# pch header file path
set(ENGINE_PRECOMPILED_HEADERS
    ${ENGINE_SOURCE_DIR}/pch.h
)
target_precompile_headers(Engine PRIVATE ${ENGINE_PRECOMPILED_HEADERS})
