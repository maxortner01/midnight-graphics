cmake_minimum_required(VERSION 3.2)

project(main CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
message("Using Vulkan version ${Vulkan_VERSION}")

set(SDL_TEST_LIBRARY OFF CACHE BOOL "")
add_subdirectory(extern/SDL)
add_subdirectory(extern/simple-lua)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE SDL3::SDL3 Vulkan::Vulkan simple-lua)

target_compile_definitions(main PRIVATE -DSOURCE_DIR="${CMAKE_SOURCE_DIR}/base")