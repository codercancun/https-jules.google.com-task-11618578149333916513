#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace neseq
{
/**
    Provides a click-free bypass crossfade for the NES-EQ plugin.

    Instead of hard-cutting audio when bypass is toggled, this class captures
    the dry signal before processing and crossfades between dry and wet over a
    short ramp (default 15 ms). This eliminates audible clicks that otherwise
    occur when the EQ state changes abruptly.

    Usage in processBlock:
        1. Call setBypassed() with the current bypass state.
        2. If shouldCaptureDry(), call captureDry() with the input buffer.
        3. If shouldRunEffect(), run the EQ processing on the buffer.
        4. Call mix() to apply the crossfade.
*/
class BypassCrossfader
{
public:
    BypassCrossfader() = default;

    void prepare (double sampleRate, int numChannels, int maxBlockSize,
                  double rampSeconds = 0.015);

    void setBypassed (bool bypassed) noexcept;

    /** True when we need to capture the dry signal for crossfading. */
    bool shouldCaptureDry() const noexcept { return isRamping() || isBypassed; }

    /** True when the effect should still run (either fully wet or ramping). */
    bool shouldRunEffect() const noexcept { return isRamping() || ! isBypassed; }

    /** Capture the dry signal before processing overwrites the buffer. */
    void captureDry (const juce::AudioBuffer<float>& buffer);

    /** Apply the wet/dry crossfade to the processed buffer. */
    void mix (juce::AudioBuffer<float>& wetBuffer);

private:
    bool isRamping() const noexcept { return wetGain.isSmoothing(); }

    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> wetGain { 1.0f };
    bool isBypassed = false;
};
} // namespace neseq
