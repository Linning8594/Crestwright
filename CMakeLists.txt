# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (c) 2026 Linning

cmake_minimum_required(VERSION 3.22)
project(Crestwright VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# statically link the MSVC runtime so the plugin has no VC++ redist dependency
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(JUCE_DIR "D:/JUCE" CACHE PATH "Path to a JUCE 9 source checkout")
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at ${JUCE_DIR}. Pass -DJUCE_DIR=<path> to your JUCE source directory.")
endif()
add_subdirectory("${JUCE_DIR}" JUCE)

juce_add_plugin(Crestwright
    COMPANY_NAME "Linning"
    BUNDLE_ID "com.linning.crestwright"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Rtau
    PLUGIN_CODE Crsw
    FORMATS VST3 Standalone
    PRODUCT_NAME "Crestwright")

target_sources(Crestwright PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/PhaseNetwork.h)

juce_add_binary_data(CrestwrightData SOURCES
    Assets/Fonts/Inter-Regular.ttf
    Assets/Fonts/Inter-Medium.ttf
    Assets/Fonts/Inter-SemiBold.ttf
    Assets/Fonts/InterDisplay-SemiBold.ttf)

target_compile_definitions(Crestwright PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Crestwright PRIVATE
    CrestwrightData
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
