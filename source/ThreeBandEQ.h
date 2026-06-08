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

    All coefficient updates happen on the audio thread via ``update`` but only
    rebuild the biquad coefficients when the target gain for a band actually
    changes beyond kGainEpsilonDb. Per-sample smoothing ramps the gain values
    to avoid zipper noise during automation.
*/
class ThreeBandEQ
{
public:
    ThreeBandEQ() = default;

    /** Prepare the EQ for playback. Must be called before process.
        @param spec         The process spec (sample rate, block size, channels).
        @param rampSeconds  Smoothing time in seconds for gain changes (default 20 ms).
    */
    void prepare (const juce::dsp::ProcessSpec& spec, float rampSeconds = 0.02f);
    void reset();

    /** Set target gains. Smoothing handles the transition. Called on the audio
        thread before ``process``. */
    void update (float lowGainDb, float midGainDb, float highGainDb);

    /** Processes the supplied context in place, applying per-sample smoothed
        coefficient updates when gains are ramping. */
    template <typename ProcessContext>
    void process (const ProcessContext& context)
    {
        if (! isSmoothing())
        {
            chain.process (context);
            return;
        }

        auto& block = context.getOutputBlock();
        const auto numSamples = static_cast<int> (block.getNumSamples());

        for (int i = 0; i < numSamples; ++i)
        {
            advanceSmoothing();

            float sample = block.getSample (0, i);
            sample = processSingleSample (sample);
            block.setSample (0, i, sample);
        }
    }

    /** Returns true if any band's gain is still ramping. */
    bool isSmoothing() const noexcept
    {
        return smoothedLow.isSmoothing()
            || smoothedMid.isSmoothing()
            || smoothedHigh.isSmoothing();
    }

    static constexpr float kLowFreqHz  = 200.0f;
    static constexpr float kMidFreqHz  = 1000.0f;
    static constexpr float kHighFreqHz = 5000.0f;
    static constexpr float kMidQ       = 0.9f;

private:
    enum BandIndex { Low = 0, Mid, High };

    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;
    using Chain = juce::dsp::ProcessorChain<Filter, Filter, Filter>;

    Chain chain;

    double sampleRate = 44100.0;

    // Smoothed gain values for zipper-free automation.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedLow  { 0.0f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedMid  { 0.0f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedHigh { 0.0f };

    // Cached current dB values for the epsilon check.
    float currentLowDb  = 0.0f;
    float currentMidDb  = 0.0f;
    float currentHighDb = 0.0f;

    static constexpr float kGainEpsilonDb = 1.0e-3f;

    void updateBand (BandIndex band, float gainDb);
    void advanceSmoothing();
    float processSingleSample (float sample);
};
} // namespace neseq
