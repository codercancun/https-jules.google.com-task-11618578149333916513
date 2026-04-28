#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

#include "NESLookAndFeel.h"

#include <array>

namespace neseq
{
class NESEQAudioProcessor;

class SpectrumAnalyzer final : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumAnalyzer (NESEQAudioProcessor& processor);

    void paint (juce::Graphics& g) override;
    void resized() override {}

private:
    void timerCallback() override;

    NESEQAudioProcessor& audioProcessor;

    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    static constexpr int kNumBars = 32;
    std::array<float, kNumBars> barLevels {};
    std::array<float, kNumBars> peakLevels {};

    static constexpr float kDecayRate = 0.92f;
    static constexpr float kPeakDecay = 0.97f;

    void drawBars (juce::Graphics& g, juce::Rectangle<int> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzer)
};

} // namespace neseq
