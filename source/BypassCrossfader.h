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
    allocation occurs. Once the fade has settled to either extreme the
    ``shouldRunEffect`` / ``shouldCaptureDry`` helpers short-circuit the
    per-block work so the CPU cost is zero when fully bypassed and minimal
    when fully active.
*/
class BypassCrossfader
{
public:
    BypassCrossfader() = default;

    void prepare (double sampleRate, int numChannels, int maximumBlockSize,
                  double rampTimeSeconds = 0.05);

    void setBypassed (bool bypassed) noexcept;

    bool isFullyBypassed() const noexcept;
    bool isFullyActive()   const noexcept;
    bool isSmoothing()     const noexcept;
    float getCurrentDryMix() const noexcept;

    bool shouldRunEffect()  const noexcept;
    bool shouldCaptureDry() const noexcept;

    void captureDry (const juce::AudioBuffer<float>& buffer) noexcept;
    void mix (juce::AudioBuffer<float>& buffer) noexcept;

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dryMix { 0.0f };
    juce::AudioBuffer<float> dryBuffer;
    double sampleRate  = 44100.0;
    double rampSeconds = 0.05;
};
} // namespace neseq
