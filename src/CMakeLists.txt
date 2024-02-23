file(GLOB SRCS ./Visualization/*.cpp)
file(GLOB HDRS ./Visualization/*.h)

add_executable(openwindow ${HDRS} ${SRCS} openwindow.cpp Model.cpp Mesh.h Model.h )   # 创建可执行文件目标
target_link_libraries(openwindow glad assimp stb_image glfw ${OPENGL_LIBRARIES})

