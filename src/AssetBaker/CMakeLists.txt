set(CMAKE_CXX_STANDARD 20)

add_executable (assetbaker "main.cpp")

set_property(TARGET assetbaker PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:extra>")

target_include_directories(assetbaker PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/external/glm")

target_link_libraries(assetbaker PUBLIC tiny_obj_loader stb_image json lz4 assetlib fastgltf::fastgltf)