cmake_minimum_required(VERSION 3.22)
project(MertsVocalEnhancer VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: istersen kendi indirdigin JUCE klasorunu ver (-DMERT_JUCE_PATH=C:/JUCE),
# vermezsen otomatik olarak GitHub'dan indirilir (GitHub Actions boyle calisir).
set(MERT_JUCE_PATH "" CACHE PATH "Yerel JUCE klasoru (opsiyonel)")

if(MERT_JUCE_PATH)
    add_subdirectory(${MERT_JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.12
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(MertsVocalEnhancer
    COMPANY_NAME "Mert"
    PLUGIN_MANUFACTURER_CODE Mert
    PLUGIN_CODE Mvem
    FORMATS VST3
    PRODUCT_NAME "Merts Ultimate Vocal Enchancer"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(MertsVocalEnhancer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

target_compile_definitions(MertsVocalEnhancer PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MertsVocalEnhancer
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
