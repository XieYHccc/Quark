set(EDITOR_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

FILE(GLOB_RECURSE EDITOR_SRC_FILES ${EDITOR_SRC_DIR} "*.cpp" "*.c" "*.h")
source_group(TREE ${EDITOR_SRC_DIR} FILES ${EDITOR_SRC_FILES})

add_executable(QuarkEditor ${EDITOR_SRC_FILES})
target_link_libraries(QuarkEditor quark imguizmo)
target_include_directories(QuarkEditor PUBLIC ${EDITOR_SRC_DIR})

set_target_properties(QuarkEditor PROPERTIES FOLDER "Tools")

set(POST_BUILD_COMMANDS
  COMMAND ${CMAKE_COMMAND} -E make_directory "${BINARY_ROOT_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${QUARK_ROOT_DIR}/BuiltinResources" "${BINARY_ROOT_DIR}/BuiltinResources"
  COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini" "${BINARY_ROOT_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:QuarkEditor>/" "${BINARY_ROOT_DIR}"
)

add_custom_command(TARGET QuarkEditor ${POST_BUILD_COMMANDS})