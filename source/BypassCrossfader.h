#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace neseq
{
/**
    Small real-time safe helper that performs a click-free crossfade between
    the original (dry) input of an audio block and the processed (wet) output
    whenever the effect is toggled on or off.

    Usage in ``processBlock``:

    @code
        crossfader.setBypassed (bypassParam->load() > 0.5f);

        if (crossfader.shouldCaptureDry())
            crossfader.captureDry (buffer);

        if (crossfader.shouldRunEffect())
            eq.process (buffer);                 // in-place wet processing

        crossfader.mix (buffer);                 // fade dry and wet together
    @endcode

    The dry buffer is preallocated in ``prepare`` so no audio-thread
    allocation occurs. Once the fade has completed the helper exposes
    ``isFullyBypassed`` / ``isFullyActive`` so the caller can skip unneeded
    work (copying the dry buffer, or running the effect chain at all).
*/
class BypassCrossfader
{
public:
    BypassCrossfader() = default;

    /** Must be called before any processing. Preallocates the dry buffer. */
    void prepare (double sampleRate,
                  int    numChannels,
                  int    maximumBlockSize,
                  double rampTimeSeconds = 0.015);

    /** Target bypass state. 0 ms later the internal smoother starts moving
        toward that state; it reaches it ``rampTimeSeconds`` after the
        change. */
    void setBypassed (bool bypassed) noexcept;

    bool isFullyBypassed() const noexcept;
    bool isFullyActive()   const noexcept;
    bool isSmoothing()     const noexcept;

    /** Returns the current dry-mix value (0 = fully wet, 1 = fully dry). For
        tests / debugging. */
    float getCurrentDryMix() const noexcept;

    /** True if the effect needs to actually run this block. Equivalent to
        "we're either not fully bypassed, or we will be mixing some wet into
        the output during a fade". */
    bool shouldRunEffect() const noexcept;

    /** True if the dry signal needs to be captured this block. Equivalent to
        "we're either fully bypassed, or we will be mixing some dry into the
        output during a fade". */
    bool shouldCaptureDry() const noexcept;

    /** Copies ``buffer`` into the internal dry store. Call before running
        the effect in-place on ``buffer``. */
    void captureDry (const juce::AudioBuffer<float>& buffer) noexcept;

    /** Crossfades the wet ``buffer`` with the stored dry copy, advancing the
        smoother per-sample so the fade is audibly glitch-free. */
    void mix (juce::AudioBuffer<float>& buffer) noexcept;

private:
    juce::AudioBuffer<float> dryBuffer;

    // 0.0 = fully wet, 1.0 = fully dry.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dryMix;

    double rampSeconds = 0.015;
    double sampleRate  = 44100.0;
};
} // namespace neseq
