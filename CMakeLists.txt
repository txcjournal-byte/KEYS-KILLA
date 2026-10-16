cmake_minimum_required(VERSION 3.22)

# Version lives here only
project(KEYS_KILLA VERSION 0.46.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(MSVC)   # PDB symbols in Release so crash dumps are readable
    add_compile_options($<$<CONFIG:Release>:/Zi>)
    add_link_options($<$<CONFIG:Release>:/DEBUG> $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
endif()

# --- Identity ---------------------------------------------------------------
# Maker name TrapVST (the manufacturer code stays Kila). NEVER change the CODES after release,
# hosts identify saved projects by them.
set(KK_COMPANY_NAME      "TrapVST")
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
    PRODUCT_NAME "EVOLVE"
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
    assets/lab_bg.jpg assets/lab_white.png assets/lab_black.png assets/lab_icons.png)

# v0.32: VOODOO / EFFECTOR / DIGGA KILLA are separate plugins again (their sources stay in Source/plugins, not built here)
set(KK_MODULE_SOURCES)
set(KK_MODULE_DATA)

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
    JUCE_USE_FLAC=1
    JUCE_PLUGINHOST_VST3=1
    $<$<PLATFORM_ID:Darwin>:JUCE_PLUGINHOST_AU=1>
    JUCE_DISPLAY_SPLASH_SCREEN=0
    KK_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(KeysKilla
    PRIVATE
        KeysKillaAssets ${KK_MODULE_DATA}
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# v0.42 EVOLVE FX PRO: the same plugin as an EFFECT (FL's mixer lists only effects) - for the MASTER / a track:
# MIX LAB + COACH on your whole beat, LISTEN hears another plugin's sound.  Its own codes, never change them.
juce_add_plugin(EvolveFX
    COMPANY_NAME "${KK_COMPANY_NAME}"
    BUNDLE_ID com.808killa.evolvefx
    PLUGIN_MANUFACTURER_CODE ${KK_MANUFACTURER_CODE}
    PLUGIN_CODE Kkfx
    PRODUCT_NAME "EVOLVE FX PRO"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Fx
    AU_MAIN_TYPE kAudioUnitType_MusicEffect
    FORMATS VST3 AU
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(EvolveFX PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Presets.cpp)
target_compile_definitions(EvolveFX PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_USE_FLAC=1
    JUCE_PLUGINHOST_VST3=1
    $<$<PLATFORM_ID:Darwin>:JUCE_PLUGINHOST_AU=1>
    JUCE_DISPLAY_SPLASH_SCREEN=0
    KK_FX_BUILD=1
    KK_VERSION_STRING="${PROJECT_VERSION}")
target_link_libraries(EvolveFX
    PRIVATE
        KeysKillaAssets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
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
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_USE_MP3AUDIOFORMAT=1 JUCE_USE_FLAC=1 JUCE_PLUGINHOST_VST3=1 KK_TEST_BUILD=1 JUCE_MODAL_LOOPS_PERMITTED=1
        JucePlugin_Name="EVOLVE" JucePlugin_IsSynth=1 JucePlugin_VersionString="${PROJECT_VERSION}")
    # the tests keep several KEYS KILLA instances on the stack: Windows' default 1 MB stack is too small
    if (MSVC)
        target_link_options(KeysKillaTests PRIVATE /STACK:67108864)
    endif()
    target_link_libraries(KeysKillaTests PRIVATE
        KeysKillaAssets ${KK_MODULE_DATA}
        juce::juce_audio_utils juce::juce_dsp juce::juce_opengl
        juce::juce_recommended_config_flags)
endif()

# =============================================================================
# CHROME VAULT by TrapWin - a separate preset-bank instrument (Source/vault, docs/VAULT.md).
# Its own company / codes - NEVER change the codes after release, hosts identify saved projects by them.
# Its own version (separate from EVOLVE's project version); CI reads it from this line for the installers.
set(CV_VERSION "1.0.0")
juce_add_plugin(ChromeVault
    VERSION ${CV_VERSION}
    COMPANY_NAME "TrapWin"
    BUNDLE_ID com.trapwin.chromevault
    PLUGIN_MANUFACTURER_CODE Trpw
    PLUGIN_CODE Cvlt
    PRODUCT_NAME "CHROME VAULT"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    # --- vault-ui --- TRUE: the search box must get typing on macOS (FALSE = the window ignores key presses), arrows + PC keys
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    FORMATS VST3 AU Standalone
    COPY_PLUGIN_AFTER_BUILD FALSE)

# every .cpp in Source/vault (engine, processor, editor and its UI parts) + the shared EVOLVE factory sounds
file(GLOB CV_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/vault/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Source/vault/models/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/vault/ui/*.cpp)
# the sound engine alone (no processor / editor): for console tools
set(CV_ENGINE_SOURCES Source/vault/VaultTypes.cpp Source/vault/VaultConvert.cpp Source/vault/VaultFx.cpp Source/vault/VaultMod.cpp
    Source/vault/VaultEngine.cpp Source/vault/VaultLibrary.cpp Source/vault/VaultSampler.cpp)
file(GLOB CV_MODEL_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Source/vault/models/*.cpp)
set(CV_SHARED_SOURCES Source/Presets.cpp)

# the preset bank: every resources/vault/*.cvb is embedded (loaded in file-name order). No files = the plugin
# falls back to a small starter bank made from the EVOLVE factory sounds.
file(GLOB CV_BANK_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resources/vault/*.cvb)
set(CV_DATA_LIB)
set(CV_BANK_DEF CV_HAS_BANK=0)
if(CV_BANK_FILES)
    juce_add_binary_data(ChromeVaultData HEADER_NAME VaultData.h NAMESPACE VaultData SOURCES ${CV_BANK_FILES})
    set(CV_DATA_LIB ChromeVaultData)
    set(CV_BANK_DEF CV_HAS_BANK=1)
endif()

target_sources(ChromeVault PRIVATE ${CV_SOURCES} ${CV_SHARED_SOURCES})
target_compile_definitions(ChromeVault PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_OGGVORBIS=1
    ${CV_BANK_DEF}
    CV_VERSION_STRING="${CV_VERSION}")
target_link_libraries(ChromeVault
    PRIVATE
        ${CV_DATA_LIB}
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(KK_BUILD_TESTS)
    # VaultTests: serializer, offline render, mono lows, declick, state (tests/VaultTests.cpp)
    juce_add_console_app(VaultTests PRODUCT_NAME "VaultTests")
    target_sources(VaultTests PRIVATE tests/VaultTests.cpp ${CV_SOURCES} ${CV_SHARED_SOURCES})
    # --- vault models --- the physical-model unit tests (VaultTests -models)
    target_sources(VaultTests PRIVATE tests/VaultModelTests.cpp)
    # the frozen v2 engine: the reference the v3 conversion is checked against (VaultTests, "v3 conversion")
    target_sources(VaultTests PRIVATE tests/legacy/LegacyVaultEngine.cpp tests/VaultV3Tests.cpp)
    # --- end vault models ---
    target_compile_definitions(VaultTests PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_USE_OGGVORBIS=1 JUCE_MODAL_LOOPS_PERMITTED=1 ${CV_BANK_DEF} CV_TEST_BUILD=1
        JucePlugin_Name="CHROME VAULT" JucePlugin_IsSynth=1 JucePlugin_VersionString="${CV_VERSION}")
    target_link_libraries(VaultTests PRIVATE
        ${CV_DATA_LIB}
        juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()

# --- vault bank ---------------------------------------------------------------
# VaultBankGen: assembles the factory bank (v3) resources/vault/factory.cvb from the preset recipes (tools/vault/recipes,
# gate tools/vault/recipes/RecipeGate.h): VaultBankGen -recipe all -qa -out resources/vault/factory.cvb (resources/vault/README.md).
# -v1 runs the legacy v1 synth-bank generator. The generated .cvb is committed, so normal builds / CI never need to run it.
if(KK_BUILD_TESTS)
    juce_add_console_app(VaultBankGen PRODUCT_NAME "VaultBankGen")
    # v2 preset recipes: one file per sound designer (tools/vault/recipes/*.cpp, framework tools/vault/recipes/Recipe.h),
    # run with VaultBankGen -recipe <name>|all (gate: tools/vault/recipes/RecipeGate.h)
    file(GLOB CV_RECIPE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/vault/recipes/*.cpp)
    target_sources(VaultBankGen PRIVATE tools/vault/BankGen.cpp ${CV_ENGINE_SOURCES} ${CV_MODEL_SOURCES} ${CV_RECIPE_SOURCES} Source/Presets.cpp)
    target_compile_definitions(VaultBankGen PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_USE_OGGVORBIS=1 CV_HAS_BANK=0)
    target_link_libraries(VaultBankGen PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
# --- end vault bank -------------------------------------------------------------

# --- vault library --------------------------------------------------------------
# VaultLibBuild: builds the CHROME VAULT sample library (library.json + audio/*.ogg) from the CC0 source recordings, driven by
# tools/vault/library_map.json (tools/vault/LibBuild.cpp, docs/VAULT.md "Sample library"). Never part of the plugin build.
if(KK_BUILD_TESTS)
    juce_add_console_app(VaultLibBuild PRODUCT_NAME "VaultLibBuild")
    target_sources(VaultLibBuild PRIVATE tools/vault/LibBuild.cpp ${CV_ENGINE_SOURCES} ${CV_MODEL_SOURCES} Source/Presets.cpp)
    target_compile_definitions(VaultLibBuild PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_USE_OGGVORBIS=1 JUCE_USE_FLAC=1 CV_HAS_BANK=0)
    target_link_libraries(VaultLibBuild PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
# --- end vault library ----------------------------------------------------------

# --- vault models ---------------------------------------------------------------
# VaultModelLab: demos, CPU benchmark and measurement renders of the physical models (tools/vault/ModelLab.cpp,
# analysis: tools/vault/models_measure.py, docs/VAULT_MODELS.md). Never part of the plugin build.
if(KK_BUILD_TESTS)
    juce_add_console_app(VaultModelLab PRODUCT_NAME "VaultModelLab")
    target_sources(VaultModelLab PRIVATE tools/vault/ModelLab.cpp ${CV_ENGINE_SOURCES} ${CV_MODEL_SOURCES} Source/Presets.cpp)
    target_compile_definitions(VaultModelLab PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_USE_OGGVORBIS=1 CV_HAS_BANK=0)
    target_link_libraries(VaultModelLab PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
# --- end vault models -----------------------------------------------------------
