cmake_minimum_required(VERSION 3.15)
project(octobass-core VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/BassProcessor.cpp
    src/Crossover.cpp
)

add_library(octobass-core STATIC ${SOURCES})

target_include_directories(octobass-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(octobass-core
    PUBLIC
        octobir-core
)

if(APPLE)
    target_compile_options(octobass-core PRIVATE -Wall -Wextra)
elseif(WIN32)
    target_compile_definitions(octobass-core PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
    )
endif()

set_target_properties(octobass-core PROPERTIES
    PUBLIC_HEADER "include/octobass-core/BassProcessor.hpp;include/octobass-core/Crossover.hpp;include/octobass-core/Types.hpp"
    POSITION_INDEPENDENT_CODE ON
)

option(BUILD_OCTOBASS_CORE_TESTS "Build octobass-core unit tests" OFF)
if(BUILD_OCTOBASS_CORE_TESTS)
    add_subdirectory(tests)
endif()
