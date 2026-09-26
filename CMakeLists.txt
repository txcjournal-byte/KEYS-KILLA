cmake_minimum_required(VERSION 3.22)

# Version lives here only
project(KEYS_KILLA VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# --- Identity ---------------------------------------------------------------
# PLACEHOLDERS for the test build. Replace with the real brand + codes
# (manufacturer code must match 808 KILLA) before the first public release,
# and never change them after that.
set(KK_COMPANY_NAME     "KILLA"  CACHE STRING "Manufacturer name")
set(KK_MANUFACTURER_CODE "Klla"  CACHE STRING "4-char manufacturer code")
set(KK_PLUGIN_CODE       "Kky1"  CACHE STRING "4-char plugin code")

# --- JUCE -------------------------------------------------------------------
set(KK_JUCE_DIR "" CACHE PATH "Local JUCE checkout (optional)")
if(KK_JUCE_DIR)
    add_subdirectory(${KK_JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(KeysKilla
    COMPANY_NAME "${KK_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${KK_MANUFACTURER_CODE}
    PLUGIN_CODE ${KK_PLUGIN_CODE}
    PRODUCT_NAME "KEYS KILLA"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    FORMATS VST3 AU Standalone
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(KeysKilla PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Presets.cpp)

target_compile_definitions(KeysKilla PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(KeysKilla
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Offline render test (sanity check of all factory presets) ---------------
option(KK_BUILD_TESTS "Build render test" ON)
if(KK_BUILD_TESTS)
    juce_add_console_app(KeysKillaTests PRODUCT_NAME "KeysKillaTests")
    target_sources(KeysKillaTests PRIVATE
        tests/RenderTest.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Presets.cpp)
    target_compile_definitions(KeysKillaTests PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 KK_TEST_BUILD=1
        JucePlugin_Name="KEYS KILLA" JucePlugin_IsSynth=1)
    target_link_libraries(KeysKillaTests PRIVATE
        juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
