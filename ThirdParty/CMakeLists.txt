set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(THIRD_PARTY_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(THIRD_PARTY_FLODER "ThirdParty")

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
set(SPDLOG_USE_STD_FORMAT ON)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/spdlog)
set_property(TARGET spdlog PROPERTY FOLDER ${THIRD_PARTY_FLODER})

# spirv
set(SPIRV_REFLECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/spirv_reflect")
add_library(spirv_reflect STATIC "${SPIRV_REFLECT_DIR}/spirv_reflect.c" 
    "${SPIRV_REFLECT_DIR}/spirv_reflect.h")
target_include_directories(spirv_reflect SYSTEM PUBLIC ${SPIRV_REFLECT_DIR})
set_target_properties(spirv_reflect PROPERTIES FOLDER ${THIRD_PARTY_FLODER})

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
set_target_properties(imgui PROPERTIES FOLDER ${THIRD_PARTY_FLODER})

# glfw
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw)
set_target_properties(glfw PROPERTIES FOLDER ${THIRD_PARTY_FLODER}/glfw)
set_target_properties(update_mappings PROPERTIES FOLDER ${THIRD_PARTY_FLODER}/glfw)

# tinyobjloader
set(TINYOBJLOADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader")
add_library(tiny_obj_loader STATIC "${TINYOBJLOADER_DIR}/tiny_obj_loader.h" 
    "${TINYOBJLOADER_DIR}/tiny_obj_loader.cpp")
target_include_directories(tiny_obj_loader PUBLIC ${TINYOBJLOADER_DIR})
set_target_properties(tiny_obj_loader PROPERTIES FOLDER ${THIRD_PARTY_FLODER})

# stb image
set(STB_IMAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/stb_image")
add_library(stb_image STATIC "${STB_IMAGE_DIR}/stb_image.h" 
    "${STB_IMAGE_DIR}/stb_image_write.h"
    "${STB_IMAGE_DIR}/stb_image.cpp")
target_include_directories(stb_image PUBLIC ${STB_IMAGE_DIR})
set_target_properties(stb_image PROPERTIES FOLDER ${THIRD_PARTY_FLODER})

# tinygltf
set(TINYGLTF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tinygltf)
add_library(tinygltf STATIC "${TINYGLTF_DIR}/tiny_gltf.cpp" 
    "${TINYGLTF_DIR}/tiny_gltf.h" 
    "${TINYGLTF_DIR}/stb_image.h" 
    "${TINYGLTF_DIR}/stb_image_write.h")
target_include_directories(tinygltf SYSTEM PUBLIC ${TINYGLTF_DIR})
set_target_properties(tinygltf PROPERTIES FOLDER ${THIRD_PARTY_FLODER})

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
set_target_properties(ktx PROPERTIES FOLDER ${THIRD_PARTY_FLODER})

# basisu
set(BASISU_DIR ${CMAKE_CURRENT_SOURCE_DIR}/basisu)
set(BASISU_SOURCES
    ${BASISU_DIR}/transcoder/basisu_transcoder.cpp
    ${BASISU_DIR}/zstd/zstd.c)

set(BASISU_INCLUDE_DIRS
    ${BASISU_DIR}/transcoder
    ${BASISU_DIR}/zstd)

add_library(basisu STATIC ${BASISU_SOURCES})
target_include_directories(basisu SYSTEM PUBLIC ${BASISU_INCLUDE_DIRS})
if(MSVC)
    # 使用MSVC编译器的选项
    target_compile_options(basisu PRIVATE /wd4996) # 用来忽略“已弃用”警告
else()
    # 对GCC或Clang使用的选项
    target_compile_options(basisu PRIVATE -Wno-deprecated-declarations)
    target_compile_options(basisu PRIVATE -Wno-deprecated-builtins)
endif()
set_target_properties(basisu PROPERTIES FOLDER ${THIRD_PARTY_FLODER})

# yaml-cpp
set(YAML_DIR ${THIRD_PARTY_ROOT}/yaml-cpp)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Disable YAML tools")
set(BUILD_TESTING OFF CACHE BOOL "Disable ctest") # TODO: This doesn't belong here, move it in the future
add_subdirectory(${YAML_DIR})
set_target_properties(yaml-cpp PROPERTIES FOLDER ${THIRD_PARTY_FLODER})

# nfd-extended
set(NFD_EXTENDED_DIR ${THIRD_PARTY_ROOT}/nfd-extended)
add_subdirectory(${NFD_EXTENDED_DIR})
set_target_properties(nfd PROPERTIES FOLDER ${THIRD_PARTY_FLODER})


# imguizmo
set(IMGUIZMO_DIR ${THIRD_PARTY_ROOT}/imguizmo)
set(IMGUIZMO_SOURCES
	${IMGUIZMO_DIR}/ImGuizmo.cpp
	${IMGUIZMO_DIR}/ImGuizmo.h
)
add_library(imguizmo STATIC ${IMGUIZMO_SOURCES})
target_include_directories(imguizmo SYSTEM PUBLIC ${IMGUIZMO_DIR})
target_link_libraries(imguizmo imgui)
set_target_properties(imguizmo PROPERTIES FOLDER ${THIRD_PARTY_FLODER})

#glslang
option(ENABLE_SPVREMAPPER OFF)
option(ENABLE_GLSLANG_BINARIES OFF)
set(ENABLE_HLSL ON)
option(ENABLE_OPT OFF)
option(BUILD_TESTING OFF)
option(BUILD_EXTERNAL OFF)
if(IOS)
    option(ENABLE_PCH OFF)
endif ()

add_subdirectory(glslang)


if (TARGET GenericCodeGen) #TODO: Will be removed in the future, currently here for compatability
    set_target_properties(GenericCodeGen PROPERTIES FOLDER ${THIRD_PARTY_FLODER}/glslang)
endif()
if (TARGET MachineIndependent)
    set_target_properties(MachineIndependent PROPERTIES FOLDER ${THIRD_PARTY_FLODER}/glslang)
endif()
if (TARGET OSDependent)
    set_target_properties(OSDependent PROPERTIES FOLDER ${THIRD_PARTY_FLODER}/glslang)
endif()
if (TARGET SPIRV)
    set_target_properties(SPIRV PROPERTIES FOLDER ${THIRD_PARTY_FLODER}/glslang)
endif()

if (NOT TARGET glslang-default-resource-limits)
    add_library(glslang-default-resource-limits
                glslang/StandAlone/ResourceLimits.cpp)

    target_include_directories(glslang-default-resource-limits
        SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glslang)
endif()

if(NOT MSVC)
    target_compile_options(glslang PRIVATE
        "-Wno-logical-op-parentheses"
        "-Wno-unused-parameter")

    target_compile_options(SPIRV PRIVATE
        "-Wno-logical-op-parentheses"
        "-Wno-unused-parameter")
endif()

set_target_properties(glslang PROPERTIES FOLDER ${THIRD_PARTY_FLODER}/glslang)
set_target_properties(glslang-default-resource-limits PROPERTIES FOLDER ${THIRD_PARTY_FLODER}/glslang)