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

    Per-band gain is internally smoothed with a linear ramp (default 20 ms)
    to eliminate zipper noise when a parameter is dragged. During ``process``
    the buffer is split into small sub-blocks; each sub-block reads the
    current smoothed gain, rebuilds coefficients only when the gain has
    actually moved (outside a small dB epsilon), and then processes. Once
    a smoother has reached its target value coefficient construction stops
    entirely, matching the previous zero-overhead behaviour on static gains.

    Coefficient updates use ``juce::dsp::IIR::ArrayCoefficients`` and write
    directly into the existing ``Coefficients`` object to avoid heap
    allocation on the audio thread.
*/
class ThreeBandEQ
{
public:
    ThreeBandEQ() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    /** Sets the target gain (in dB) for each band. The filter coefficients
        are not rebuilt here — they follow the internal smoothers during
        ``process``. */
    void update (float lowGainDb, float midGainDb, float highGainDb);

    /** Snaps the internal smoothers to the given target gains, bypassing the
        ramp. Intended for tests and for restoring saved state where we do
        not want to audibly sweep from zero. */
    void snap (float lowGainDb, float midGainDb, float highGainDb);

    /** Sets how long (in seconds) a gain change takes to ramp to target.
        Takes effect on the next call to ``prepare``. */
    void setSmoothingTime (float seconds) noexcept;

    /** Processes the supplied context in place. */
    template <typename ProcessContext>
    void process (const ProcessContext& context)
    {
        auto&& outBlock = context.getOutputBlock();
        const auto total = outBlock.getNumSamples();

        size_t pos = 0;
        while (pos < total)
        {
            const auto remaining = total - pos;
            const auto thisChunk = remaining < static_cast<size_t> (kSubBlockSize)
                                       ? remaining
                                       : static_cast<size_t> (kSubBlockSize);

            advanceSmoothersAndRebuild (static_cast<int> (thisChunk));

            auto sub = outBlock.getSubBlock (pos, thisChunk);
            juce::dsp::ProcessContextReplacing<float> subCtx (sub);
            chain.process (subCtx);

            pos += thisChunk;
        }
    }

    // Fixed band centre frequencies. Tuned to be musical on typical programme
    // material rather than matched to a particular hardware unit.
    static constexpr float kLowFreqHz  = 200.0f;
    static constexpr float kMidFreqHz  = 1000.0f;
    static constexpr float kHighFreqHz = 5000.0f;
    static constexpr float kMidQ       = 0.9f;

    // Granularity at which smoothed gains are re-read inside ``process``. At
    // 48 kHz this is ~0.67 ms of audio per coefficient update during a ramp.
    static constexpr int kSubBlockSize = 32;

private:
    enum BandIndex { Low = 0, Mid, High };

    using Filter       = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;
    using Chain        = juce::dsp::ProcessorChain<Filter, Filter, Filter>;
    using SmoothedDb   = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

    Chain chain;

    double sampleRate          = 44100.0;
    float  smoothingTimeSec    = 0.02f;

    SmoothedDb smoothedLow;
    SmoothedDb smoothedMid;
    SmoothedDb smoothedHigh;

    float currentLowDb  = 0.0f;
    float currentMidDb  = 0.0f;
    float currentHighDb = 0.0f;

    void advanceSmoothersAndRebuild (int numSamples);
    void rebuildBand (BandIndex band, float gainDb);
};
} // namespace neseq
