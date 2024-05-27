FILE(GLOB_RECURSE SandBoxSources ${CMAKE_SOURCE_DIR}/SandBox "*.cpp" "*.h")
add_executable(SandBox ${SandBoxSources})

target_link_libraries(SandBox Engine)
target_include_directories(SandBox PUBLIC ${CMAKE_SOURCE_DIR})