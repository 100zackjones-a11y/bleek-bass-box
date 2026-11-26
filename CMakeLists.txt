cmake_minimum_required(VERSION 3.22)
# Release builds pass -DBLEEK_VERSION=x.y.z (taken from the git tag in CI)
set(BLEEK_VERSION "1.0.0" CACHE STRING "Plugin version")
project(BleekBassBox VERSION ${BLEEK_VERSION})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE 8 automatically if not present locally in ./JUCE
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(BleekBassBox
    COMPANY_NAME "BLEEK STATE AUDIO"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE BlkS
    PLUGIN_CODE BlkB
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "BLEEK BASS BOX"
)

target_sources(BleekBassBox
    PRIVATE
        Source/MidiExporter.h
        Source/BassEngine.h
        Source/BassEngine.cpp
        Source/MonoSampler.h
        Source/MonoSampler.cpp
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
)

target_compile_definitions(BleekBassBox
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(BleekBassBox
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
