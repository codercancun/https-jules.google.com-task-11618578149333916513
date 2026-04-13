cmake_minimum_required(VERSION 3.20)
project(VintageEq VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(
    juce
    URL https://github.com/juce-framework/JUCE/archive/refs/tags/7.0.9.zip
)
FetchContent_MakeAvailable(juce)

# For headless testing on CI, disable JUCE browser and related plugins if not needed
# target_compile_definitions(juce::juce_gui_extra INTERFACE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)

juce_add_plugin(VintageEq
    PLUGIN_MANUFACTURER_CODE Juce
    PLUGIN_CODE Vteq
    FORMATS VST3 AU
    PRODUCT_NAME "VintageEq"
    VST3_AUTO_MANIFEST FALSE
)

target_compile_definitions(VintageEq PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

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

enable_testing()

add_executable(VintageEqTests
    tests/VintageEqTests.cpp
    VintageEqAudioProcessor.cpp
)

target_compile_definitions(VintageEqTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_compile_features(VintageEqTests PRIVATE cxx_std_20)

target_link_libraries(VintageEqTests PRIVATE
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_audio_utils
)

add_test(NAME VintageEqTests COMMAND VintageEqTests)
