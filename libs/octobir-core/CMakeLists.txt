cmake_minimum_required(VERSION 3.15)
project(octobir-core VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/AudioBuffer.cpp
    src/IRLoader.cpp
    src/IRProcessor.cpp
)

add_library(octobir-core STATIC ${SOURCES})

target_include_directories(octobir-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/WDL/WDL
)

target_include_directories(octobir-core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party
)

set(WDL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/WDL/WDL)

target_sources(octobir-core PRIVATE
    ${WDL_DIR}/convoengine.cpp
    ${WDL_DIR}/fft.c
    ${WDL_DIR}/resample.cpp
)

if(APPLE)
    target_compile_options(octobir-core PRIVATE -Wall -Wextra)
elseif(WIN32)
    target_compile_definitions(octobir-core PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

set_target_properties(octobir-core PROPERTIES
    PUBLIC_HEADER "include/octobir-core/IRProcessor.hpp;include/octobir-core/IRLoader.hpp;include/octobir-core/AudioBuffer.hpp;include/octobir-core/Types.hpp"
)
