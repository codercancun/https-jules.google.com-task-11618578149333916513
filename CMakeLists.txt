cmake_minimum_required(VERSION 3.22)

project(NES_EQ
    VERSION 0.1.0
    LANGUAGES C CXX
    DESCRIPTION "NES-EQ: 8-bit themed 3-band EQ audio plugin"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fetch JUCE
include(FetchContent)
set(JUCE_VERSION "7.0.12" CACHE STRING "JUCE release tag to fetch")
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG ${JUCE_VERSION}
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(juce)

# Plugin formats: VST3 everywhere; AU only on Apple platforms (it requires the
# Audio Unit SDK that ships with Xcode / macOS).
set(NES_EQ_FORMATS VST3)
if(APPLE)
    list(APPEND NES_EQ_FORMATS AU)
endif()

juce_add_plugin(NES_EQ
    COMPANY_NAME             "NESEQ"
    PLUGIN_MANUFACTURER_CODE "NsEq"
    PLUGIN_CODE              "NsEQ"
    FORMATS                  ${NES_EQ_FORMATS}
    PRODUCT_NAME             "NES-EQ"
    BUNDLE_ID                "com.neseq.neseq"
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD  FALSE
    VST3_CATEGORIES          "Fx" "EQ"
    AU_MAIN_TYPE             "kAudioUnitType_Effect"
)

juce_generate_juce_header(NES_EQ)

target_sources(NES_EQ PRIVATE
    source/PluginProcessor.cpp
    source/PluginProcessor.h
    source/PluginEditor.cpp
    source/PluginEditor.h
    source/ThreeBandEQ.cpp
    source/ThreeBandEQ.h
    source/NESLookAndFeel.cpp
    source/NESLookAndFeel.h
    source/NESComponents.cpp
    source/NESComponents.h
)

target_compile_definitions(NES_EQ PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(NES_EQ PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_plugin_client
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_core
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Slightly stricter warnings on GCC/Clang without making JUCE's own headers
# fail to compile.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(NES_EQ PRIVATE -Wno-unused-parameter)
endif()

# ---------------------------------------------------------------------------
# Tests (console app; built by default, runnable via CTest)
# ---------------------------------------------------------------------------
option(NES_EQ_BUILD_TESTS "Build the NES-EQ DSP unit tests" ON)

if(NES_EQ_BUILD_TESTS)
    enable_testing()

    juce_add_console_app(NES_EQ_Tests
        PRODUCT_NAME "NES-EQ-Tests"
    )

    juce_generate_juce_header(NES_EQ_Tests)

    target_sources(NES_EQ_Tests PRIVATE
        tests/TestsMain.cpp
        tests/ThreeBandEQTests.cpp
        source/ThreeBandEQ.cpp
        source/ThreeBandEQ.h
    )

    target_compile_definitions(NES_EQ_Tests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )

    target_link_libraries(NES_EQ_Tests PRIVATE
        juce::juce_core
        juce::juce_dsp
        juce::juce_audio_basics
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )

    add_test(NAME NES_EQ_Tests COMMAND NES_EQ_Tests)
endif()
