FILE(GLOB_RECURSE SandBoxSources ${CMAKE_SOURCE_DIR}/src/SandBox "*.cpp" "*.h")
add_executable(SandBox ${SandBoxSources})

target_link_libraries(SandBox EngineCore)
target_include_directories(SandBox PUBLIC ${CMAKE_SOURCE_DIR})