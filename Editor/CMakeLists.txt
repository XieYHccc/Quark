
set(EDITOR_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

FILE(GLOB_RECURSE EDITOR_SRC_FILES ${EDITOR_SRC_DIR} "*.cpp" "*.c" "*.h")

source_group(TREE ${EDITOR_SRC_DIR} FILES "${EDITOR_SRC_FILES}")

add_executable(Editor ${EDITOR_SRC_FILES})

target_link_libraries(Editor quark imguizmo)
target_include_directories(Editor PUBLIC ${EDITOR_SRC_DIR})