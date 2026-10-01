cmake_minimum_required(VERSION 3.22)

# Version lives here only
project(KEYS_KILLA VERSION 0.15.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(MSVC)   # PDB symbols in Release so crash dumps are readable
    add_compile_options($<$<CONFIG:Release>:/Zi>)
    add_link_options($<$<CONFIG:Release>:/DEBUG> $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
endif()

# --- Identity ---------------------------------------------------------------
# Same maker as 808 KILLA (manufacturer code Kila). NEVER change these after release,
# hosts identify saved projects by them.
set(KK_COMPANY_NAME      "808 KILLA")
set(KK_MANUFACTURER_CODE "Kila")
set(KK_PLUGIN_CODE       "Kkey")

# --- JUCE -------------------------------------------------------------------
set(KK_JUCE_DIR "" CACHE PATH "Local JUCE checkout (optional)")
if(KK_JUCE_DIR)
    add_subdirectory(${KK_JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(KeysKilla
    COMPANY_NAME "${KK_COMPANY_NAME}"
    BUNDLE_ID com.808killa.keyskilla
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

# Skin bitmaps cut from the design mockups (tools/make_skin_assets.py)
juce_add_binary_data(KeysKillaAssets SOURCES
    assets/lab_bg.jpg assets/lab_white.png assets/lab_black.png assets/lab_icons.png
    assets/voodoo_factory.json assets/effector_factory.json)

# Modules taken from the other KILLA plugins (each keeps its own namespace)
set(KK_MODULE_SOURCES
    Source/modules/voodoo/Engine.cpp Source/modules/voodoo/Morph.cpp Source/modules/voodoo/Preset.cpp
    Source/modules/voodoo/PresetLibrary.cpp Source/modules/voodoo/TimeBuffer.cpp Source/modules/voodoo/Trigger.cpp
    Source/modules/voodoo/Modules/Drive.cpp Source/modules/voodoo/Modules/Filter.cpp Source/modules/voodoo/Modules/Gate.cpp
    Source/modules/voodoo/Modules/LoFi.cpp Source/modules/voodoo/Modules/Pitch.cpp Source/modules/voodoo/Modules/Space.cpp
    Source/modules/k808/Engine.cpp)
file(GLOB_RECURSE KK_EFFECTOR_SOURCES CONFIGURE_DEPENDS Source/modules/effector/*.cpp)
list(APPEND KK_MODULE_SOURCES ${KK_EFFECTOR_SOURCES})

target_sources(KeysKilla PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Presets.cpp
    ${KK_MODULE_SOURCES})

target_compile_definitions(KeysKilla PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_DISPLAY_SPLASH_SCREEN=0
    KK_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(KeysKilla
    PRIVATE
        KeysKillaAssets
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
        Source/Presets.cpp
        ${KK_MODULE_SOURCES})
    target_compile_definitions(KeysKillaTests PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_USE_MP3AUDIOFORMAT=1 KK_TEST_BUILD=1 JUCE_MODAL_LOOPS_PERMITTED=1
        JucePlugin_Name="KEYS KILLA" JucePlugin_IsSynth=1)
    target_link_libraries(KeysKillaTests PRIVATE
        KeysKillaAssets
        juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
