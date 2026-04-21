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
    changes. Coefficient building is non-allocating — ``juce::dsp::IIR``
    coefficient construction uses a fixed-size ``juce::dsp::IIR::Coefficients``
    object backed by a small internal array.
*/
class ThreeBandEQ
{
public:
    ThreeBandEQ() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    /** Called on the audio thread before ``process``. Only rebuilds filter
        coefficients for bands whose target gain has changed. */
    void update (float lowGainDb, float midGainDb, float highGainDb);

    /** Processes the supplied context in place. */
    template <typename ProcessContext>
    void process (const ProcessContext& context)
    {
        chain.process (context);
    }

    // Fixed band centre frequencies. Tuned to be musical on typical programme
    // material rather than matched to a particular hardware unit.
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

    float currentLowDb  = 0.0f;
    float currentMidDb  = 0.0f;
    float currentHighDb = 0.0f;

    void updateBand (BandIndex band, float gainDb);
};
} // namespace neseq
