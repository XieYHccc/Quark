file(GLOB SRCS ./Visualization/*.cpp)
file(GLOB HDRS ./Visualization/*.h)

add_executable(openwindow ${HDRS} ${SRCS} 
               mesh_importer.cpp 
               openwindow.cpp 
               material_importer.cpp
               mesh_renderer.cpp
               texture.cpp
               )   # 创建可执行文件目标

target_link_libraries(openwindow 
                      tiny_obj_loader
                      glad 
                      assimp 
                      stb_image 
                      glfw 
                      ${OPENGL_LIBRARIES} 
                      RTTR::Core_Lib)

