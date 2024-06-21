cmake_minimum_required(VERSION 3.2)

project(main C CXX)

set(CMAKE_CXX_STANDARD 20)

option(MN_BUILD_LIB "Build the midnight-graphics library" ON)
if (MN_BUILD_LIB)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "")
    add_subdirectory(extern/VMA)
    add_subdirectory(extern/SDL)
    add_subdirectory(extern/simple-lua)
endif()

option(MN_BUILD_EXEC "Build the test executable" ON)
if (MN_BUILD_EXEC)
    add_executable(main main.cpp)
    target_link_libraries(main PRIVATE midnight-graphics)
    target_include_directories(main PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/midnight)
    target_compile_definitions(main PRIVATE -DSOURCE_DIR="${CMAKE_SOURCE_DIR}/tests")
endif()

option(MN_BUILD_DOCS "Build the documentation" OFF)
if (MN_BUILD_DOCS)
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
        include(ExternalProject)
        ExternalProject_Add(doxygenawesome
            GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
            DOWNLOAD_EXTRACT_TIMESTAMP ON
            SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/doxygen-awesome"
            DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/doxygen-awesome"
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
            CONFIGURE_COMMAND ""
            GIT_TAG origin/main
        )

        # set input and output files
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        # request to configure the file
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        message("Doxygen build started")

        # note the option ALL which allows to build the docs together with the application
        add_custom_target( doc_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM )
    else (DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif()