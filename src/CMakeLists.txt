cmake_minimum_required(VERSION 3.2)

project(graphics CXX)

# Find vulkan and its path so we can locate shaderc
find_package(Vulkan REQUIRED)
get_filename_component(VULKAN_PATH ${Vulkan_LIBRARIES} DIRECTORY)

find_library(SHADERC shaderc_combinedd HINTS ${VULKAN_PATH})
if (NOT SHADERC)
    find_library(SHADERC shaderc_combined HINTS ${VULKAN_PATH})

    if (NOT SHADERC)
        message(FATAL_ERROR "No version of shaderc found")
    endif()

else()
    message("Using shaderc_combinedd")
endif()

message("Using Vulkan version ${Vulkan_VERSION}")

set(SPIRV_REFLECT
    ${CMAKE_SOURCE_DIR}/extern/spirv-reflect/spirv_reflect.c
    ${CMAKE_SOURCE_DIR}/extern/spirv-reflect/spirv_reflect.h)

add_library(midnight-graphics SHARED
    ${CMAKE_SOURCE_DIR}/src/Graphics/Backend/Instance.cpp
    ${CMAKE_SOURCE_DIR}/src/Graphics/Backend/Device.cpp
    ${CMAKE_SOURCE_DIR}/src/Graphics/Backend/Command.cpp
    ${CMAKE_SOURCE_DIR}/src/Graphics/Backend/Sync.cpp

    ${CMAKE_SOURCE_DIR}/src/Graphics/Window.cpp
    ${CMAKE_SOURCE_DIR}/src/Graphics/Pipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/Graphics/RenderFrame.cpp
    ${CMAKE_SOURCE_DIR}/src/Graphics/Buffer.cpp
    ${CMAKE_SOURCE_DIR}/src/Graphics/Image.cpp
    ${CMAKE_SOURCE_DIR}/src/Graphics/Model.cpp

    ${CMAKE_SOURCE_DIR}/src/Math/Angle.cpp

    ${SPIRV_REFLECT})

target_link_libraries(midnight-graphics PRIVATE SDL3::SDL3 Vulkan::Vulkan ${SHADERC} GPUOpen::VulkanMemoryAllocator simple-lua)
target_include_directories(midnight-graphics PRIVATE 
    ${CMAKE_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/include/midnight 
    ${CMAKE_SOURCE_DIR}/extern/spirv-reflect)
    target_compile_definitions(midnight-graphics PRIVATE -DMN_BUILD)