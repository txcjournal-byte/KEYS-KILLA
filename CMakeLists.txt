cmake_minimum_required(VERSION 3.22)

# TrapWin Lucky Notes - random melodies that actually fit (VST3 + AU + Standalone).
# Version lives here only.
project(LUCKY_NOTES VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# LN_TESTS_ONLY=ON: only the engine + its console test (no plugin, no GUI) - e.g. on a Linux box without X11 headers.
option(LN_TESTS_ONLY "Build only the engine tests" OFF)
set(LN_JUCE_DIR "" CACHE PATH "Local JUCE checkout (optional)")

if(LN_JUCE_DIR)
    add_subdirectory(${LN_JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

set(LN_COMMON_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    LN_VERSION_STRING="${PROJECT_VERSION}")

# --- engine: plain C++ (melody generator, MIDI writer, preview synth) ---------------------------------------
add_library(LuckyNotesEngine STATIC
    Source/engine/MelodyGenerator.cpp
    Source/engine/MidiWriter.cpp
    Source/engine/PreviewEngine.cpp)
target_include_directories(LuckyNotesEngine PUBLIC Source/engine)

# --- engine tests (generator, MIDI round trip via juce::MidiFile, offline preview render) --------------------
juce_add_console_app(LuckyNotesTests PRODUCT_NAME "LuckyNotesTests")
target_sources(LuckyNotesTests PRIVATE tests/EngineTests.cpp)
target_compile_definitions(LuckyNotesTests PRIVATE ${LN_COMMON_DEFS})
target_link_libraries(LuckyNotesTests PRIVATE
    LuckyNotesEngine
    juce::juce_audio_basics
    juce::juce_recommended_config_flags)

if(NOT LN_TESTS_ONLY)
    juce_add_binary_data(LuckyNotesFonts
        NAMESPACE LuckyNotesBinary
        HEADER_NAME LuckyNotesBinary.h
        SOURCES
            resources/fonts/PressStart2P-Regular.ttf
            resources/fonts/VT323-Regular.ttf
            resources/fonts/IBMPlexMono-Regular.ttf
            resources/fonts/IBMPlexMono-Bold.ttf)

    set(LN_FORMATS VST3 Standalone)
    if(APPLE)
        set(LN_FORMATS VST3 AU Standalone)
    endif()

    # NEVER change the codes after release: hosts identify saved projects by them.
    juce_add_plugin(LuckyNotes
        VERSION ${PROJECT_VERSION}
        COMPANY_NAME "TrapWin"
        COMPANY_WEBSITE "https://trapwin.com"
        BUNDLE_ID com.trapwin.luckynotes
        PLUGIN_MANUFACTURER_CODE Trpw
        PLUGIN_CODE Lkyn
        PRODUCT_NAME "Lucky Notes"
        DESCRIPTION "Random melodies that actually fit"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT TRUE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES Instrument Generator
        AU_MAIN_TYPE kAudioUnitType_MusicDevice
        FORMATS ${LN_FORMATS}
        COPY_PLUGIN_AFTER_BUILD FALSE)

    set(LN_PLUGIN_SOURCES
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/PhosphorLookAndFeel.cpp)

    target_sources(LuckyNotes PRIVATE ${LN_PLUGIN_SOURCES})
    target_compile_definitions(LuckyNotes PUBLIC ${LN_COMMON_DEFS})
    target_link_libraries(LuckyNotes
        PRIVATE
            LuckyNotesEngine
            LuckyNotesFonts
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    # headless editor screenshot for the shop page: LuckyNotesSnapshot <out.png> [scale]
    juce_add_console_app(LuckyNotesSnapshot PRODUCT_NAME "LuckyNotesSnapshot")
    target_sources(LuckyNotesSnapshot PRIVATE tests/Snapshot.cpp ${LN_PLUGIN_SOURCES})
    target_compile_definitions(LuckyNotesSnapshot PRIVATE ${LN_COMMON_DEFS}
        LN_SNAPSHOT_BUILD=1 JucePlugin_Name="Lucky Notes" JucePlugin_IsSynth=1 JucePlugin_ProducesMidiOutput=1
        JucePlugin_WantsMidiInput=0 JucePlugin_IsMidiEffect=0 JucePlugin_VersionString="${PROJECT_VERSION}")
    target_link_libraries(LuckyNotesSnapshot PRIVATE
        LuckyNotesEngine
        LuckyNotesFonts
        juce::juce_audio_processors
        juce::juce_gui_extra
        juce::juce_recommended_config_flags)
endif()
