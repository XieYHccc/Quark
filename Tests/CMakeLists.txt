# job system test
add_executable(JobSystem_Test ./JobSystem_Test.cpp)
target_link_libraries(JobSystem_Test quark)
#  target_include_directories(JobSystem_Test PUBLIC ${CMAKE_SOURCE_DIR})
set_target_properties(JobSystem_Test PROPERTIES FOLDER "Tests")

# gltf viewer
add_quark_test(GLTFViewer ./GLTFViewer.cpp)

# skybox test
add_executable(Skybox_Test ./Skybox_Test.cpp)
target_link_libraries(Skybox_Test quark)
set_target_properties(Skybox_Test PROPERTIES FOLDER "Tests")

