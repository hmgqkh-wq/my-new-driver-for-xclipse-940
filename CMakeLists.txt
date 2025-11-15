cmake_minimum_required(VERSION 3.15)
project(ExynosX940FullPort LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
option(ENABLE_SHADER_BUILD "Compile shaders at build time (requires glslc)" ON)

include_directories(include)

file(GLOB_RECURSE SRC src/*.cpp src/*.c)
add_library(exynosfull SHARED ${SRC})

if(ENABLE_SHADER_BUILD)
    find_program(GLSLC glslc)
    find_program(XXD xxd)
    if(GLSLC AND XXD)
        file(GLOB SHADERS assets/shaders/*.comp)
        foreach(SH ${SHADERS})
            get_filename_component(NAME ${SH} NAME_WE)
            set(SPV ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.spv)
            add_custom_command(OUTPUT ${SPV}
                COMMAND ${GLSLC} -fshader-stage=compute ${CMAKE_CURRENT_SOURCE_DIR}/${SH} -o ${SPV}
                DEPENDS ${SH}
            )
            set(HEADER ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.spv.h)
            add_custom_command(OUTPUT ${HEADER}
                COMMAND ${XXD} -i ${SPV} > ${HEADER}
                DEPENDS ${SPV}
            )
            list(APPEND SPV_HEADERS ${HEADER})
        endforeach()
        add_custom_target(compile_shaders DEPENDS ${SPV_HEADERS})
        add_dependencies(exynosfull compile_shaders)
        include_directories(${CMAKE_CURRENT_BINARY_DIR})
    else()
        message(WARNING "glslc or xxd not found; runtime compilation fallback will be required.")
    endif()
endif()

find_path(VULKAN_INCLUDE vulkan/vulkan.h)
find_library(VULKAN_LIB NAMES vulkan vk)
if(VULKAN_INCLUDE) target_include_directories(exynosfull PRIVATE ${VULKAN_INCLUDE}) endif()
if(VULKAN_LIB) target_link_libraries(exynosfull PRIVATE ${VULKAN_LIB}) endif()
