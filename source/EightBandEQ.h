#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>

namespace neseq
{
/**
    A real-time safe eight band EQ used by the NES-EQ plugin.

    The chain is:
        Band 0 : low-shelf  (60 Hz)
        Band 1 : peak/bell  (150 Hz)
        Band 2 : peak/bell  (400 Hz)
        Band 3 : peak/bell  (800 Hz)
        Band 4 : peak/bell  (1.6 kHz)
        Band 5 : peak/bell  (3.2 kHz)
        Band 6 : peak/bell  (6.4 kHz)
        Band 7 : high-shelf (12 kHz)

    All coefficient updates happen on the audio thread via ``update`` but only
    rebuild the biquad coefficients when the target gain for a band actually
    changes. Coefficient building is non-allocating.
*/
class EightBandEQ
{
public:
    static constexpr int kNumBands = 8;

    EightBandEQ() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    /** Called on the audio thread before ``process``. Only rebuilds filter
        coefficients for bands whose target gain has changed. */
    void update (const std::array<float, kNumBands>& gainsDb);

    /** Processes the supplied context in place. */
    template <typename ProcessContext>
    void process (const ProcessContext& context)
    {
        for (auto& filter : filters)
            filter.process (context);
    }

    // Band centre frequencies (Hz).
    static constexpr std::array<float, kNumBands> kFrequencies {{
        60.0f, 150.0f, 400.0f, 800.0f, 1600.0f, 3200.0f, 6400.0f, 12000.0f
    }};

    // Band labels for the UI.
    static constexpr std::array<const char*, kNumBands> kLabels {{
        "SUB", "BASS", "LO", "MID", "HI-M", "PRES", "BRIL", "AIR"
    }};

    static constexpr float kPeakQ = 0.9f;

private:
    enum FilterType { LowShelf, Peak, HighShelf };

    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    std::array<Filter, kNumBands> filters;
    std::array<float, kNumBands>  currentDb {};
    double sampleRate = 44100.0;

    void updateBand (int band, float gainDb);
    FilterType typeForBand (int band) const;
};
} // namespace neseq
