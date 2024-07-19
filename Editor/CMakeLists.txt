FILE(GLOB_RECURSE EditorSources ${CMAKE_SOURCE_DIR}/Editor "*.cpp" "*.h")
add_executable(Editor ${EditorSources})

target_link_libraries(Editor quark)
target_include_directories(Editor PUBLIC ${CMAKE_SOURCE_DIR})