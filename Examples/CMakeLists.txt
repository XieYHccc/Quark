add_executable(TrangleApp ./Trangle/Trangle.cpp)
target_link_libraries(TrangleApp Engine)
target_include_directories(TrangleApp PUBLIC ${CMAKE_SOURCE_DIR})
