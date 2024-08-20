
set(EDITOR_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

FILE(GLOB_RECURSE EditorSources ${EDITOR_SRC_DIR} "*.cpp" "*.h")

source_group(TREE ${EDITOR_SRC_DIR} FILES ${EditorSources})

add_executable(Editor ${EditorSources})

target_link_libraries(Editor quark imguizmo)
target_include_directories(Editor PUBLIC ${EDITOR_SRC_DIR})