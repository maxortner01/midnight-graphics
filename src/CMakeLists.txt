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
    ${MIDNIGHT_BASE_DIR}/extern/spirv-reflect/spirv_reflect.c
    ${MIDNIGHT_BASE_DIR}/extern/spirv-reflect/spirv_reflect.h)

set(IMGUI_SOURCES
    ${MIDNIGHT_BASE_DIR}/extern/imgui/imgui.h
    ${MIDNIGHT_BASE_DIR}/extern/imgui/imgui.cpp
    ${MIDNIGHT_BASE_DIR}/extern/imgui/imgui_draw.cpp
    ${MIDNIGHT_BASE_DIR}/extern/imgui/imgui_demo.cpp
    ${MIDNIGHT_BASE_DIR}/extern/imgui/imgui_internal.h
    ${MIDNIGHT_BASE_DIR}/extern/imgui/imgui_tables.cpp
    ${MIDNIGHT_BASE_DIR}/extern/imgui/imgui_widgets.cpp
    ${MIDNIGHT_BASE_DIR}/extern/imgui/imstb_rectpack.h
    ${MIDNIGHT_BASE_DIR}/extern/imgui/imstb_textedit.h
    ${MIDNIGHT_BASE_DIR}/extern/imgui/imstb_truetype.h

    ${MIDNIGHT_BASE_DIR}/extern/imgui/backends/imgui_impl_sdl3.cpp
    ${MIDNIGHT_BASE_DIR}/extern/imgui/backends/imgui_impl_sdl3.h
    ${MIDNIGHT_BASE_DIR}/extern/imgui/backends/imgui_impl_vulkan.cpp
    ${MIDNIGHT_BASE_DIR}/extern/imgui/backends/imgui_impl_vulkan.h
)

add_library(midnight-graphics SHARED
    ${MIDNIGHT_BASE_DIR}/src/Graphics/Backend/Instance.cpp
    ${MIDNIGHT_BASE_DIR}/src/Graphics/Backend/Device.cpp
    ${MIDNIGHT_BASE_DIR}/src/Graphics/Backend/Command.cpp
    ${MIDNIGHT_BASE_DIR}/src/Graphics/Backend/Sync.cpp

    ${MIDNIGHT_BASE_DIR}/src/Graphics/Window.cpp
    ${MIDNIGHT_BASE_DIR}/src/Graphics/Pipeline.cpp
    ${MIDNIGHT_BASE_DIR}/src/Graphics/RenderFrame.cpp
    ${MIDNIGHT_BASE_DIR}/src/Graphics/Buffer.cpp
    ${MIDNIGHT_BASE_DIR}/src/Graphics/Texture.cpp
    ${MIDNIGHT_BASE_DIR}/src/Graphics/Image.cpp
    ${MIDNIGHT_BASE_DIR}/src/Graphics/Mesh.cpp
    ${MIDNIGHT_BASE_DIR}/src/Graphics/Keyboard.cpp
    ${MIDNIGHT_BASE_DIR}/src/Graphics/Descriptor.cpp

    ${MIDNIGHT_BASE_DIR}/src/Math/Angle.cpp

    ${SPIRV_REFLECT}
    ${IMGUI_SOURCES})

target_link_libraries(midnight-graphics PRIVATE SDL3::SDL3 Vulkan::Vulkan ${SHADERC} GPUOpen::VulkanMemoryAllocator simple-lua)
target_include_directories(midnight-graphics 
    PUBLIC
        ${MIDNIGHT_BASE_DIR}/include 
        ${MIDNIGHT_BASE_DIR}/include/midnight 
        ${MIDNIGHT_BASE_DIR}/extern/imgui
    PRIVATE 
        ${MIDNIGHT_BASE_DIR}/extern/spirv-reflect
        ${MIDNIGHT_BASE_DIR}/extern/stb)
    target_compile_definitions(midnight-graphics PRIVATE -DMN_BUILD)