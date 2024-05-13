set(CMAKE_CXX_STANDARD 20)

add_library (assetlib STATIC 
    "IO.h"
    "IO.cpp"
    "Texture.h"
    "Texture.cpp"
)

target_include_directories(assetlib PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/external/lz4"
    "${CMAKE_SOURCE_DIR}/external/nlohmann-json")

target_link_libraries(assetlib PRIVATE json lz4)