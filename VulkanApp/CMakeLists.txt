FILE(GLOB_RECURSE VulkanAppSources ${CMAKE_SOURCE_DIR}/SandBox "*.cpp" "*.h")
add_executable(VulkanApp ${VulkanAppSources})

target_link_libraries(VulkanApp Engine)
target_include_directories(VulkanApp PUBLIC ${CMAKE_SOURCE_DIR})