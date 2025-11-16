cmake_minimum_required(VERSION 3.15)
project(OctobIR VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(../../third_party/JUCE ${CMAKE_CURRENT_BINARY_DIR}/JUCE)

add_subdirectory(../../libs/octob-ir-core ${CMAKE_CURRENT_BINARY_DIR}/octob-ir-core)

set(FORMATS VST3 AU Standalone)

juce_add_plugin(OctobIR
    COMPANY_NAME "OctobIR"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Octb
    PLUGIN_CODE Obir
    FORMATS ${FORMATS}
    PRODUCT_NAME "OctobIR")

target_sources(OctobIR PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(OctobIR
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(OctobIR
    PRIVATE
        juce::juce_audio_utils
        octob-ir-core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
