FILE(GLOB_RECURSE TestBedSources ${CMAKE_SOURCE_DIR}/TestBed "*.cpp" "*.h")
add_executable(TestBed ${TestBedSources})

target_link_libraries(TestBed quark)
target_include_directories(TestBed PUBLIC ${CMAKE_SOURCE_DIR})