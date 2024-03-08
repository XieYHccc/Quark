file(GLOB RENDER_SRCS "./render/*.cpp")
file(GLOB RENDER_HDRS "./render/*.h")
file(GLOB PHYSICS_HDRS "./physics/*.h")
file(GLOB PHYSICS_SRCS "./physics/*.cpp")
file(GLOB GEOMETRY_HDRS "./geometry/*.h")
file(GLOB GEOMETRY_SRCS "./geometry/*.cpp")
file(GLOB SCENE_HDRS "./scene/*.h")
file(GLOB SCENE_SRCS "./scene/*.cpp")
file(GLOB_RECURSE OBJECT_HDRS "./Object/*.h")
file(GLOB_RECURSE OBJECT_SRCS "./Object/*.cpp")
file(GLOB INPUT_HDRS "./input/*.h")
file(GLOB INPUT_SRCS "./input/*.cpp")


add_executable(application ${RENDER_SRCS} ${RENDER_HDRS}
               ${GEOMETRY_HDRS} ${GEOMETRY_SRCS}
               ${PHYSICS_HDRS} ${PHYSICS_SRCS}
               ${SCENE_HDRS} ${SCENE_SRCS}
               ${OBJECT_HDRS} ${OBJECT_SRCS}
               ${INPUT_HDRS} ${INPUT_SRCS}
               ./Application/main.cpp
               ./Application/Application.h ./Application/Application.cpp
)

target_link_libraries(application 
                      tiny_obj_loader
                      glad 
                      stb_image 
                      glfw 
                      ${OPENGL_LIBRARIES} 
                      )
