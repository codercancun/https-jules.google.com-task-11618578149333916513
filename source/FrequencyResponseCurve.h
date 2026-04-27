#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

#include "NESLookAndFeel.h"
#include "ThreeBandEQ.h"

namespace neseq
{
/**
    Real-time frequency response curve for the 3-band EQ. Draws the combined
    magnitude response of the three filter stages on a dark recessed panel
    with a log-frequency x-axis (20 Hz–20 kHz) and a dB y-axis (±18 dB).

    Recomputes the response from the filter coefficients whenever the band
    gain parameters change, driven by a 30 Hz timer repaint.
*/
class FrequencyResponseCurve final : public juce::Component,
                                      private juce::Timer
{
public:
    FrequencyResponseCurve (juce::AudioProcessorValueTreeState& apvts,
                            double initialSampleRate = 48000.0);
    ~FrequencyResponseCurve() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setSampleRate (double sr);

private:
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvts;
    double sampleRate;

    static constexpr float kMinFreqHz  = 20.0f;
    static constexpr float kMaxFreqHz  = 20000.0f;
    static constexpr float kMinDb      = -18.0f;
    static constexpr float kMaxDb      = 18.0f;
    static constexpr int   kNumPoints  = 256;

    std::array<float, kNumPoints> magnitudesDb {};

    void recomputeResponse();

    float freqToX (float freq, float width) const;
    float dbToY (float db, float height) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyResponseCurve)
};
} // namespace neseq
