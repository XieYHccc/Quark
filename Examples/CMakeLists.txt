add_executable(TrangleApp ./Trangle/Trangle.cpp)
target_link_libraries(TrangleApp quark)
target_include_directories(TrangleApp PUBLIC ${CMAKE_SOURCE_DIR})


add_executable(Texture ./Texture/Texture.cpp)
target_link_libraries(Texture quark)
target_include_directories(Texture PUBLIC ${CMAKE_SOURCE_DIR})

