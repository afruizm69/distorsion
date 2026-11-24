cmake_minimum_required(VERSION 3.22)

# Mac: binario universal (Apple Silicon + Intel)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Arquitecturas macOS")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "macOS minimo")

project(DistorsionExtrema VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
# Windows: runtime estatico, asi el .exe/.vst3 no necesita instalar Visual C++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: usa una copia local si se indica (-DJUCE_PATH=...), si no la descarga.
set(JUCE_PATH "" CACHE PATH "Ruta local a JUCE (opcional)")
if(JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(DEX_FORMATS VST3 AU Standalone)
else()
    set(DEX_FORMATS VST3 Standalone)
endif()

juce_add_plugin(DistorsionExtrema
    COMPANY_NAME "Distorsion Extrema"
    BUNDLE_ID "com.distorsionextrema.plugin"
    PLUGIN_MANUFACTURER_CODE Dext
    PLUGIN_CODE Dxt1
    FORMATS ${DEX_FORMATS}
    PRODUCT_NAME "DISTORSION EXTREMA"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect)

target_sources(DistorsionExtrema PRIVATE
    Source/Parameters.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(DistorsionExtrema PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

# Solo para compilaciones de prueba en Linux sin todas las librerias del sistema
if(UNIX AND NOT APPLE AND DEX_LINUX_MINIMAL)
    target_compile_definitions(DistorsionExtrema PUBLIC
        JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0
        JUCE_USE_XCURSOR=0 JUCE_USE_XSHM=0 JUCE_OPENGL=0)
endif()

option(DEX_BUILD_TESTS "Compilar el banco de pruebas" OFF)
if(DEX_BUILD_TESTS)
    juce_add_console_app(DexTests PRODUCT_NAME "DexTests")
    target_sources(DexTests PRIVATE Tests/TestMain.cpp)
    target_compile_definitions(DexTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(DexTests PRIVATE DistorsionExtrema juce::juce_audio_utils juce::juce_dsp
                                   juce::juce_recommended_config_flags)
endif()

target_link_libraries(DistorsionExtrema
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
