cmake_minimum_required(VERSION 3.15)
project(OctobIR-Monorepo VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_CORE_LIB "Build octob-ir-core library" ON)
option(BUILD_JUCE_PLUGIN "Build JUCE plugin (VST3/AU/Standalone)" ON)
option(BUILD_VCV_PLUGIN "Build VCV Rack plugin" ON)

if(BUILD_CORE_LIB)
    add_subdirectory(libs/octob-ir-core)
endif()

if(BUILD_JUCE_PLUGIN)
    add_subdirectory(plugins/juce-multiformat)
endif()

if(BUILD_VCV_PLUGIN)
    add_subdirectory(plugins/vcv-rack)
endif()
