FILE(GLOB_RECURSE SandBoxSources ${CMAKE_SOURCE_DIR}/src/SandBox "*.cpp" "*.c" "*.h")
add_executable(SandBox ${SandBoxSources})

target_link_libraries(SandBox EngineCore)