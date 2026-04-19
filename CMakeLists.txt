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

# --- Testing Setup ---
enable_testing()

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

juce_add_console_app(VintageEqTests
  PRODUCT_NAME "VintageEqTests"
)

# Bypass web browser requirements for headless CI
target_compile_definitions(VintageEqTests PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_compile_features(VintageEqTests PRIVATE cxx_std_20)

target_sources(VintageEqTests PRIVATE
  VintageEqAudioProcessor.cpp
  tests/VintageEqAudioProcessorTest.cpp
)

target_link_libraries(VintageEqTests PRIVATE
  juce::juce_audio_processors
  juce::juce_audio_utils
  juce::juce_dsp
  gtest_main
)

juce_generate_juce_header(VintageEqTests)

add_test(NAME VintageEqTests COMMAND VintageEqTests)
