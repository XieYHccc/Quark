cmake_minimum_required(VERSION 3.10)

project(third_party LANGUAGES C CXX)

# glm
add_library(glm INTERFACE)
set(GLM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm)
target_sources(glm INTERFACE ${GLM_DIR}/glm/glm.hpp)
target_include_directories(glm SYSTEM INTERFACE ${GLM_DIR})

target_compile_definitions(glm INTERFACE
    GLM_FORCE_SWIZZLE
    GLM_FORCE_RADIANS
    GLM_FORCE_CTOR_INIT
    GLM_ENABLE_EXPERIMENTAL
)
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_definitions(glm INTERFACE GLM_FORCE_CXX14)
endif()

# vma
add_library(vma INTERFACE)
set(VMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vma/include)
target_sources(vma INTERFACE ${VMA_DIR}/vk_mem_alloc.h)
target_include_directories(vma SYSTEM INTERFACE ${VMA_DIR})
target_link_libraries(vma INTERFACE Vulkan::Vulkan)

# spdlog
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/spdlog)
set_property(TARGET spdlog PROPERTY FOLDER "ThirdParty")

# spirv
set(SPIRV_REFLECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/spirv_reflect")
add_library(spirv_reflect STATIC "${SPIRV_REFLECT_DIR}/spirv_reflect.c" 
    "${SPIRV_REFLECT_DIR}/spirv_reflect.h")
target_include_directories(spirv_reflect SYSTEM PUBLIC ${SPIRV_REFLECT_DIR})
set_target_properties(spirv_reflect PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)

# imgui
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui_docking")
set(IMGUI_FILES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imconfig.h"
    "${IMGUI_DIR}/imgui.h"
    "${IMGUI_DIR}/imgui_internal.h"
    "${IMGUI_DIR}/imstb_rectpack.h"
    "${IMGUI_DIR}/imstb_textedit.h"
    "${IMGUI_DIR}/imstb_truetype.h"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.h"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_vulkan.h"
    "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp")

add_library(imgui STATIC ${IMGUI_FILES})
target_include_directories(imgui SYSTEM PUBLIC ${IMGUI_DIR})
target_link_libraries(imgui glfw Vulkan::Vulkan)
set_target_properties(imgui PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)

# glfw
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw)
set_target_properties(glfw PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)

# tinyobjloader
set(TINYOBJLOADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader")
add_library(tiny_obj_loader STATIC "${TINYOBJLOADER_DIR}/tiny_obj_loader.h" 
    "${TINYOBJLOADER_DIR}/tiny_obj_loader.cpp")
target_include_directories(tiny_obj_loader PUBLIC ${TINYOBJLOADER_DIR})
set_target_properties(tiny_obj_loader PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)

# stb image
set(STB_IMAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/stb_image")
add_library(stb_image STATIC "${STB_IMAGE_DIR}/stb_image.h" 
    "${STB_IMAGE_DIR}/stb_image_write.h"
    "${STB_IMAGE_DIR}/stb_image.cpp")
target_include_directories(stb_image PUBLIC ${STB_IMAGE_DIR})
set_target_properties(stb_image PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)

# tinygltf
set(TINYGLTF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tinygltf)
add_library(tinygltf STATIC "${TINYGLTF_DIR}/tiny_gltf.cpp" 
    "${TINYGLTF_DIR}/tiny_gltf.h" 
    "${TINYGLTF_DIR}/stb_image.h" 
    "${TINYGLTF_DIR}/stb_image_write.h")
target_include_directories(tinygltf SYSTEM PUBLIC ${TINYGLTF_DIR})
set_target_properties(tinygltf PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)

# libktx
set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ktx)
set(KTX_SOURCES
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/filestream.c
)

set(KTX_INCLUDE_DIRS
    ${KTX_DIR}/include
    ${KTX_DIR}/other_include
)

add_library(ktx STATIC ${KTX_SOURCES})

target_include_directories(ktx SYSTEM PUBLIC ${KTX_INCLUDE_DIRS})
target_link_libraries(ktx PUBLIC Vulkan::Vulkan)
set_target_properties(ktx PROPERTIES FOLDER "ThirdParty" POSITION_INDEPENDENT_CODE ON)
