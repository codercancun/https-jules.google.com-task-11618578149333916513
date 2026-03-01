cmake_minimum_required(VERSION 3.20)
project(VintageEq VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(
    juce
    URL https://github.com/juce-framework/JUCE/archive/refs/tags/7.0.9.zip
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(VintageEq
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE Vteq
    FORMATS VST3 AU
    PRODUCT_NAME "VintageEq"
)

target_sources(VintageEq PRIVATE
    VintageEqAudioProcessor.cpp
)

target_compile_features(VintageEq PRIVATE cxx_std_20)

target_link_libraries(VintageEq PRIVATE
    juce::juce_audio_plugin_client
    juce::juce_dsp
    juce::juce_audio_utils
    juce::juce_audio_processors
)
