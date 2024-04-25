cmake_minimum_required(VERSION 3.2)

project(main CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
get_filename_component(VULKAN_PATH ${Vulkan_LIBRARIES} DIRECTORY)

find_library(SHADERC shaderc_combined HINTS ${VULKAN_PATH})

message("Using Vulkan version ${Vulkan_VERSION}")

set(SDL_TEST_LIBRARY OFF CACHE BOOL "")
add_subdirectory(extern/SDL)
add_subdirectory(extern/simple-lua)

add_executable(main 
    main.cpp
    ${CMAKE_SOURCE_DIR}/src/Graphics/Backend/Instance.cpp
    ${CMAKE_SOURCE_DIR}/src/Graphics/Backend/Device.cpp
    ${CMAKE_SOURCE_DIR}/src/Graphics/Backend/Command.cpp
    ${CMAKE_SOURCE_DIR}/src/Graphics/Backend/Sync.cpp

    ${CMAKE_SOURCE_DIR}/src/Graphics/Window.cpp
    ${CMAKE_SOURCE_DIR}/src/Graphics/Pipeline.cpp
)
target_link_libraries(main PRIVATE SDL3::SDL3 Vulkan::Vulkan ${SHADERC} simple-lua)
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include/midnight)

target_compile_definitions(main PRIVATE -DSOURCE_DIR="${CMAKE_SOURCE_DIR}/base")