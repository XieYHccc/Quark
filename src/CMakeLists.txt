file(GLOB_RECURSE RENDER_SRCS "./render/*.cpp")
file(GLOB_RECURSE RENDER_HDRS "./render/*.h")
file(GLOB CORE_HDRS "./core/*.h")
file(GLOB CORE_SRCS "./core/*.cpp")

add_executable(application ${RENDER_SRCS} ${RENDER_HDRS} ${CORE_HDRS} ${CORE_SRCS}
               ./app/main.cpp ./app/viewer.cpp ./app/viewer.h
)

target_link_libraries(application 
                      tiny_obj_loader
                      glad 
                      stb_image 
                      glfw 
                      ${OPENGL_LIBRARIES} 
                      RTTR::Core_Lib)

