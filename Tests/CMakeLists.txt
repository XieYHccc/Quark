# job system test
add_executable(JobSystem_Test ./JobSystem_Test.cpp)
target_link_libraries(JobSystem_Test quark)
#  target_include_directories(JobSystem_Test PUBLIC ${CMAKE_SOURCE_DIR})
set_target_properties(JobSystem_Test PROPERTIES FOLDER "Tests")

# gltf viewer
add_quark_test(GLTFViewer ./GLTFViewer.cpp)

# texture viewer
add_quark_test(TextureViewer ./TextureViewer.cpp)

# skybox test
add_executable(Skybox_Test ./Skybox_Test.cpp)
target_link_libraries(Skybox_Test quark)
set_target_properties(Skybox_Test PROPERTIES FOLDER "Tests")

# ibl test
add_executable(IBL_Test ./IBL_Test.cpp)
target_link_libraries(IBL_Test quark)
set_target_properties(IBL_Test PROPERTIES FOLDER "Tests")