#pragma once

#include <juce_dsp/juce_dsp.h>

namespace neseq
{
/**
    A simple, real-time safe three band EQ used by the NES-EQ plugin.

    The chain is:
        Low  : low-shelf filter (default 200 Hz)
        Mid  : peak / bell filter (default 1 kHz, Q ~ 0.9)
        High : high-shelf filter (default 5 kHz)

    Gain parameters are smoothed via ``juce::SmoothedValue`` to eliminate
    zipper noise when the user moves sliders. Coefficients are rebuilt every
    block while a ramp is active and only when the smoothed value actually
    moves beyond a small epsilon.
*/
class ThreeBandEQ
{
public:
    ThreeBandEQ() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    /** Called on the audio thread before ``process``. Sets the target gains;
        smoothing ramps toward them over the configured ramp duration. */
    void update (float lowGainDb, float midGainDb, float highGainDb);

    /** Processes the supplied context in place, advancing the gain smoothers
        per-sample and updating filter coefficients as needed. */
    void process (juce::dsp::AudioBlock<float>& block);

    /** Returns the current (smoothed) gain in dB for the given band. */
    float getSmoothedGainDb (int bandIndex) const;

    /** Returns true if any band's smoother is still ramping. */
    bool isSmoothing() const;

    // Fixed band centre frequencies.
    static constexpr float kLowFreqHz  = 200.0f;
    static constexpr float kMidFreqHz  = 1000.0f;
    static constexpr float kHighFreqHz = 5000.0f;
    static constexpr float kMidQ       = 0.9f;

    // Smoothing ramp time in seconds.
    static constexpr double kSmoothingTimeSec = 0.05;

private:
    enum BandIndex { Low = 0, Mid, High, NumBands };

    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;
    using Chain = juce::dsp::ProcessorChain<Filter, Filter, Filter>;

    Chain chain;

    double sampleRate = 44100.0;
    int blockSize = 512;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedGainDb[NumBands];
    float lastAppliedDb[NumBands] = { 0.0f, 0.0f, 0.0f };

    void updateBand (BandIndex band, float gainDb);
};
} // namespace neseq
