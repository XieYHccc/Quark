FILE(GLOB_RECURSE EngineCoreSources ${CMAKE_SOURCE_DIR}/src/Engine "*.cpp" "*.c" "*.h")

add_library(EngineCore ${EngineCoreSources})

set(EXTERNAL_PATH ${CMAKE_SOURCE_DIR}/external) # path to dependencies
set(ENGINE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/Engine)
set(ENGINE_INCLUDE_DIRS
${ENGINE_SOURCE_DIR}
${EXTERNAL_PATH}/glad-4.6/include
${EXTERNAL_PATH}/glfw/include
${EXTERNAL_PATH}/stb_image_write-1.16
${EXTERNAL_PATH}/stb_image-2.28
${EXTERNAL_PATH}/tinyobjloader
${EXTERNAL_PATH}/glm
${EXTERNAL_PATH}/spdlog/include
)

target_include_directories(EngineCore PUBLIC ${ENGINE_INCLUDE_DIRS} )
target_link_libraries(EngineCore tiny_obj_loader glad stb_image glfw ${OPENGL_LIBRARIES} )

# # pch header file path
# set(ENGINE_PRECOMPILED_HEADERS
#     ${ENGINE_SOURCE_DIR}/pch.h
# )
# target_precompile_headers(EngineCore PRIVATE ${ENGINE_PRECOMPILED_HEADERS})

# TARGET_COMPILE_DEFINITIONS(EngineCore PUBLIC XENGINE_EXPORTS )