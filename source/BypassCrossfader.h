#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace neseq
{
/**
    Real-time safe helper that performs a click-free crossfade between
    dry input and processed output when the effect is toggled on or off.
*/
class BypassCrossfader
{
public:
    BypassCrossfader() = default;

    void prepare (double sampleRate, int numChannels, int maximumBlockSize,
                  double rampTimeSeconds = 0.05);

    void setBypassed (bool bypassed) noexcept;

    bool isFullyBypassed() const noexcept;
    bool shouldRunEffect()  const noexcept;
    bool shouldCaptureDry() const noexcept;

    void captureDry (const juce::AudioBuffer<float>& buffer) noexcept;
    void mix (juce::AudioBuffer<float>& buffer) noexcept;

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dryMix { 0.0f };
    juce::AudioBuffer<float> dryBuffer;
    double currentSampleRate = 44100.0;
    double rampSeconds       = 0.05;
};
} // namespace neseq
