file(GLOB_RECURSE RENDER_SRCS "./render/*.cpp")
file(GLOB_RECURSE RENDER_HDRS "./render/*.h")
file(GLOB CORE_HDRS "./core/*.h")
file(GLOB CORE_SRCS "./core/*.cpp")
file(GLOB PHYSICS_HDRS "./physics/*.h")
file(GLOB PHYSICS_SRCS "./physics/*.cpp")
file(GLOB GEOMETRY_HDRS "./geometry/*.h")
file(GLOB GEOMETRY_SRCS "./geometry/*.cpp")

add_executable(application ${RENDER_SRCS} ${RENDER_HDRS} ${CORE_HDRS} ${CORE_SRCS} 
               ${GEOMETRY_HDRS} ${GEOMETRY_SRCS}
               ${PHYSICS_HDRS} ${PHYSICS_SRCS}
               ./app/main.cpp ./app/viewer.cpp ./app/viewer.h
)

target_link_libraries(application 
                      tiny_obj_loader
                      glad 
                      stb_image 
                      glfw 
                      ${OPENGL_LIBRARIES} 
                      RTTR::Core_Lib)
