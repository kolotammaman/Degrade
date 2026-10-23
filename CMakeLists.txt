cmake_minimum_required(VERSION 3.22)
project(Degrade VERSION 1.0.0)

set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version")

add_subdirectory(JUCE)

juce_add_plugin(Degrade
    COMPANY_NAME           "Lmos"
    PLUGIN_MANUFACTURER_CODE Lmos
    PLUGIN_CODE             Degd
    FORMATS                 AU
    PRODUCT_NAME            "Degrade"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    AU_MAIN_TYPE            kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_compile_features(Degrade PUBLIC cxx_std_17)

target_sources(Degrade PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(Degrade PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(Degrade
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
