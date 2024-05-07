cmake_minimum_required(VERSION 3.2)

project(main C CXX)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(extern/VMA)

find_package(Vulkan REQUIRED)
get_filename_component(VULKAN_PATH ${Vulkan_LIBRARIES} DIRECTORY)

find_library(SHADERC shaderc_combined HINTS ${VULKAN_PATH})

message("Using Vulkan version ${Vulkan_VERSION}")

set(SDL_TEST_LIBRARY OFF CACHE BOOL "")
add_subdirectory(extern/SDL)
add_subdirectory(extern/simple-lua)

set(SPIRV_REFLECT
    ${CMAKE_SOURCE_DIR}/extern/spirv-reflect/spirv_reflect.c
    ${CMAKE_SOURCE_DIR}/extern/spirv-reflect/spirv_reflect.h)

set(SIMPLEX_NOISE
    ${CMAKE_SOURCE_DIR}/extern/SimplexNoise/src/SimplexNoise.cpp)

add_executable(main 
    main.cpp
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

    ${CMAKE_SOURCE_DIR}/game/MeshGeneration.cpp

    ${SPIRV_REFLECT} ${SIMPLEX_NOISE}
)
target_link_libraries(main PRIVATE SDL3::SDL3 Vulkan::Vulkan ${SHADERC} GPUOpen::VulkanMemoryAllocator simple-lua)
target_include_directories(main PRIVATE 
    ${CMAKE_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/include/midnight 
    ${CMAKE_SOURCE_DIR}/extern/spirv-reflect
    ${CMAKE_SOURCE_DIR}/extern/SimplexNoise/src)

target_compile_definitions(main PRIVATE -DSOURCE_DIR="${CMAKE_SOURCE_DIR}/base")