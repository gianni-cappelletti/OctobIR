cmake_minimum_required(VERSION 3.15)
project(octob-ir-core VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/AudioBuffer.cpp
    src/IRLoader.cpp
    src/IRProcessor.cpp
)

add_library(octob-ir-core STATIC ${SOURCES})

target_include_directories(octob-ir-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/iPlug2/WDL
)

target_include_directories(octob-ir-core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party
)

set(WDL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/iPlug2/WDL)

target_sources(octob-ir-core PRIVATE
    ${WDL_DIR}/convoengine.cpp
    ${WDL_DIR}/fft.c
    ${WDL_DIR}/resample.cpp
)

if(APPLE)
    target_compile_options(octob-ir-core PRIVATE -Wall -Wextra)
elseif(WIN32)
    target_compile_definitions(octob-ir-core PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

set_target_properties(octob-ir-core PROPERTIES
    PUBLIC_HEADER "include/octob-ir-core/IRProcessor.hpp;include/octob-ir-core/IRLoader.hpp;include/octob-ir-core/AudioBuffer.hpp;include/octob-ir-core/Types.hpp"
)
