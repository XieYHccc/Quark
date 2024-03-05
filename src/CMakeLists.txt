file(GLOB_RECURSE RENDER_SRCS "./render/*.cpp")
file(GLOB_RECURSE RENDER_HDRS "./render/*.h")
file(GLOB BASIC_HDRS "./basic/*.h")
file(GLOB BASIC_SRCS "./basic/*.cpp")
file(GLOB PHYSICS_HDRS "./physics/*.h")
file(GLOB PHYSICS_SRCS "./physics/*.cpp")
file(GLOB GEOMETRY_HDRS "./geometry/*.h")
file(GLOB GEOMETRY_SRCS "./geometry/*.cpp")
file(GLOB SCENE_HDRS "./scene/*.h")
file(GLOB SCENE_SRCS "./scene/*.cpp")

add_executable(application ${RENDER_SRCS} ${RENDER_HDRS} ${BASIC_HDRS} ${BASIC_SRCS} 
               ${GEOMETRY_HDRS} ${GEOMETRY_SRCS}
               ${PHYSICS_HDRS} ${PHYSICS_SRCS}
               ${SCENE_HDRS} ${SCENE_SRCS}
               ./app/main.cpp ./app/viewer.cpp ./app/viewer.h
               ./app/asset_manager.h ./app/asset_manager.cpp
)

target_link_libraries(application 
                      tiny_obj_loader
                      glad 
                      stb_image 
                      glfw 
                      ${OPENGL_LIBRARIES} 
                      )
