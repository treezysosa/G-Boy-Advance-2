cmake_minimum_required(VERSION 3.24)
project(GBoyAdvance2 VERSION 0.12.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
option(GBA_BUILD_PLUGIN "Build JUCE VST3, standalone, and integration tests" ON)
option(GBA_BUILD_BENCHMARKS "Build optional 32-voice DSP benchmark" OFF)
option(GBA_ENABLE_SANITIZERS "Address and undefined behaviour sanitizers (Clang/GCC)" OFF)
option(GBA_WINDOWS_SOFTWARE_EDITOR "Use the software renderer for Windows editor windows" ON)
include(CTest)
add_library(gba_dsp STATIC Source/dsp/SynthEngine.cpp Source/dsp/Effects.cpp)
target_include_directories(gba_dsp PUBLIC Source)
set_target_properties(gba_dsp PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(gba_dsp PRIVATE /W4 /permissive-)
else()
    target_compile_options(gba_dsp PRIVATE -Wall -Wextra -Wpedantic)
    if(GBA_ENABLE_SANITIZERS)
        target_compile_options(gba_dsp PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(gba_dsp PUBLIC -fsanitize=address,undefined)
    endif()
endif()
if(BUILD_TESTING)
    add_executable(gba2-tests Tests/EngineTests.cpp Tests/AllocationTracker.cpp)
    target_link_libraries(gba2-tests PRIVATE gba_dsp)
    add_test(NAME synthesis COMMAND gba2-tests)
    set_tests_properties(synthesis PROPERTIES TIMEOUT 180)
    add_executable(gba2-core-tests Tests/CoreSynthesisTests.cpp Tests/AllocationTracker.cpp)
    target_link_libraries(gba2-core-tests PRIVATE gba_dsp)
    add_test(NAME core-synthesis COMMAND gba2-core-tests)
    set_tests_properties(core-synthesis PROPERTIES TIMEOUT 180)
    add_executable(gba2-advanced-tests Tests/AdvancedSynthesisTests.cpp Tests/AllocationTracker.cpp)
    target_link_libraries(gba2-advanced-tests PRIVATE gba_dsp)
    add_test(NAME advanced-synthesis COMMAND gba2-advanced-tests)
    set_tests_properties(advanced-synthesis PROPERTIES TIMEOUT 180)
    add_executable(gba2-effects-tests Tests/EffectsTests.cpp Tests/AllocationTracker.cpp)
    target_link_libraries(gba2-effects-tests PRIVATE gba_dsp)
    add_test(NAME effects COMMAND gba2-effects-tests)
    set_tests_properties(effects PROPERTIES TIMEOUT 180)
    add_executable(gba2-vocal-tests Tests/VocalTests.cpp Tests/AllocationTracker.cpp)
    target_link_libraries(gba2-vocal-tests PRIVATE gba_dsp)
    add_test(NAME vocal-synthesis COMMAND gba2-vocal-tests)
    set_tests_properties(vocal-synthesis PROPERTIES TIMEOUT 180)
    add_executable(gba2-retro-tests Tests/RetroTests.cpp Tests/AllocationTracker.cpp)
    target_link_libraries(gba2-retro-tests PRIVATE gba_dsp)
    add_test(NAME retro-synthesis COMMAND gba2-retro-tests)
    set_tests_properties(retro-synthesis PROPERTIES TIMEOUT 180)
    add_executable(gba2-sample-tests Tests/SampleDspTests.cpp Tests/AllocationTracker.cpp)
    target_link_libraries(gba2-sample-tests PRIVATE gba_dsp)
    add_test(NAME sample-playback COMMAND gba2-sample-tests)
    add_executable(gba2-preset-tests Tests/PresetTests.cpp)
    target_link_libraries(gba2-preset-tests PRIVATE gba_dsp)
    add_test(NAME factory-presets COMMAND gba2-preset-tests --auditions)
    set_tests_properties(factory-presets PROPERTIES TIMEOUT 300)
endif()
if(GBA_BUILD_BENCHMARKS)
    add_executable(gba2-benchmark Tests/Benchmark.cpp)
    target_link_libraries(gba2-benchmark PRIVATE gba_dsp)
    add_executable(gba2-modal-reference-probe Tests/ModalReferenceProbe.cpp)
    target_link_libraries(gba2-modal-reference-probe PRIVATE gba_dsp)
    add_executable(gba2-performance-probe Tests/PerformanceProbe.cpp)
    target_link_libraries(gba2-performance-probe PRIVATE gba_dsp)
endif()
if(NOT GBA_BUILD_PLUGIN)
    return()
endif()
# Pin JUCE: parameter contracts and plugin behavior must not change silently.
set(GBA_JUCE_PATH "" CACHE PATH "Optional existing JUCE 8.0.6 checkout for offline builds")
if(GBA_JUCE_PATH)
    add_subdirectory("${GBA_JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()
juce_add_plugin(GBoyAdvance2
    COMPANY_NAME "Treezy Audio"
    BUNDLE_ID "audio.treezy.gboyadvance2"
    PLUGIN_MANUFACTURER_CODE Trzy
    PLUGIN_CODE GbA2
    PRODUCT_NAME "G Boy Advance 2"
    DESCRIPTION "Hybrid synthesis and sample playback instrument"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    FORMATS VST3 Standalone)
juce_generate_juce_header(GBoyAdvance2)
target_sources(GBoyAdvance2 PRIVATE
    Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/PresetManager.cpp Source/SampleManager.cpp
    Source/PluginProcessor.h Source/PluginEditor.h Source/UiSkins.h Source/PixelText.h Source/PresetManager.h)
target_compile_definitions(GBoyAdvance2 PUBLIC
    GBA_WINDOWS_SOFTWARE_EDITOR=$<BOOL:${GBA_WINDOWS_SOFTWARE_EDITOR}>
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0)
target_link_libraries(GBoyAdvance2 PRIVATE gba_dsp juce::juce_audio_utils juce::juce_audio_processors
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
if(BUILD_TESTING)
    juce_add_console_app(gba2-integration PRODUCT_NAME "GBA2 integration tests")
    juce_generate_juce_header(gba2-integration)
    target_sources(gba2-integration PRIVATE Tests/PluginTests.cpp Tests/LibraryTests.cpp Tests/SampleTests.cpp
        Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/PresetManager.cpp Source/SampleManager.cpp)
    target_compile_definitions(gba2-integration PRIVATE JucePlugin_Name="G Boy Advance 2"
        GBA_WINDOWS_SOFTWARE_EDITOR=$<BOOL:${GBA_WINDOWS_SOFTWARE_EDITOR}>
        JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=0
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(gba2-integration PRIVATE gba_dsp juce::juce_audio_utils juce::juce_audio_processors)
    add_test(NAME plugin-state-and-presets COMMAND gba2-integration)
    set_tests_properties(plugin-state-and-presets PROPERTIES TIMEOUT 180)
    if(WIN32)
        # A separate host loads the built bundle rather than linking our processor.
        juce_add_console_app(gba2-host-tests PRODUCT_NAME "GBA2 VST3 host acceptance")
        target_sources(gba2-host-tests PRIVATE Tests/HostTests.cpp)
        target_include_directories(gba2-host-tests PRIVATE Source)
        target_compile_definitions(gba2-host-tests PRIVATE JUCE_PLUGINHOST_VST3=1
            JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
            JUCE_MODAL_LOOPS_PERMITTED=1)
        target_link_libraries(gba2-host-tests PRIVATE juce::juce_audio_utils)
        add_dependencies(gba2-host-tests GBoyAdvance2_VST3)
        # Exercise JUCE's FL-specific wrapper paths in a test host. This is not FL Studio.
        add_custom_command(TARGET gba2-host-tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:gba2-host-tests>"
                "$<TARGET_FILE_DIR:gba2-host-tests>/FL64.exe")
        add_test(NAME vst3-host-acceptance COMMAND gba2-host-tests
            "$<TARGET_FILE_DIR:GBoyAdvance2_VST3>/../..")
        set_tests_properties(vst3-host-acceptance PROPERTIES TIMEOUT 180)
        add_test(NAME vst3-fl-hostname-acceptance
            COMMAND "$<TARGET_FILE_DIR:gba2-host-tests>/FL64.exe"
                "$<TARGET_FILE_DIR:GBoyAdvance2_VST3>/../.." --fl-hostname)
        set_tests_properties(vst3-fl-hostname-acceptance PROPERTIES TIMEOUT 180)
    endif()
endif()
