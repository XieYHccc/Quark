FILE(GLOB_RECURSE VulkanAppSources ${CMAKE_SOURCE_DIR}/src/SandBox "*.cpp" "*.h")
add_executable(VulkanApp ${VulkanAppSources})

target_link_libraries(VulkanApp EngineCore)