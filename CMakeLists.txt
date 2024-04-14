cmake_minimum_required(VERSION 3.2)

project(main CXX)

set(CMAKE_CXX_STANDARD 17)

set(SDL_TEST_LIBRARY OFF CACHE BOOL "")
add_subdirectory(extern/SDL)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE SDL3::SDL3)