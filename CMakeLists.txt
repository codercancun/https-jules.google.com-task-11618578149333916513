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
    FORMATS Standalone
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
find_package(PkgConfig)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
include_directories(${GTK3_INCLUDE_DIRS})
pkg_check_modules(WEBKIT2GTK REQUIRED webkit2gtk-4.1)
include_directories(${WEBKIT2GTK_INCLUDE_DIRS})
pkg_check_modules(LIBCURL REQUIRED libcurl)
include_directories(${LIBCURL_INCLUDE_DIRS})
target_link_libraries(VintageEq PRIVATE ${GTK3_LIBRARIES} ${WEBKIT2GTK_LIBRARIES} ${LIBCURL_LIBRARIES})
