#include "BypassCrossfader.h"

namespace neseq
{
void BypassCrossfader::prepare (double sampleRate, int numChannels,
                                int maximumBlockSize, double rampTimeSeconds)
{
    currentSampleRate = sampleRate;
    rampSeconds       = rampTimeSeconds;

    dryMix.reset (sampleRate, rampSeconds);
    dryMix.setCurrentAndTargetValue (0.0f);

    dryBuffer.setSize (numChannels, maximumBlockSize, false, true, true);
}

void BypassCrossfader::setBypassed (bool bypassed) noexcept
{
    dryMix.setTargetValue (bypassed ? 1.0f : 0.0f);
}

bool BypassCrossfader::isFullyBypassed() const noexcept
{
    return ! dryMix.isSmoothing() && dryMix.getTargetValue() >= 1.0f;
}

bool BypassCrossfader::shouldRunEffect() const noexcept
{
    return ! isFullyBypassed();
}

bool BypassCrossfader::shouldCaptureDry() const noexcept
{
    return dryMix.isSmoothing() || isFullyBypassed();
}

void BypassCrossfader::captureDry (const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numCh = juce::jmin (dryBuffer.getNumChannels(), buffer.getNumChannels());
    const auto numSamples = juce::jmin (dryBuffer.getNumSamples(), buffer.getNumSamples());

    for (int ch = 0; ch < numCh; ++ch)
        juce::FloatVectorOperations::copy (dryBuffer.getWritePointer (ch),
                                           buffer.getReadPointer (ch),
                                           numSamples);
}

void BypassCrossfader::mix (juce::AudioBuffer<float>& buffer) noexcept
{
    if (! dryMix.isSmoothing())
    {
        if (dryMix.getTargetValue() >= 1.0f)
        {
            const auto numCh = juce::jmin (dryBuffer.getNumChannels(),
                                           buffer.getNumChannels());
            const auto numSamples = juce::jmin (dryBuffer.getNumSamples(),
                                                buffer.getNumSamples());
            for (int ch = 0; ch < numCh; ++ch)
                juce::FloatVectorOperations::copy (buffer.getWritePointer (ch),
                                                   dryBuffer.getReadPointer (ch),
                                                   numSamples);
        }
        return;
    }

    const auto numCh = juce::jmin (dryBuffer.getNumChannels(), buffer.getNumChannels());
    const auto numSamples = juce::jmin (dryBuffer.getNumSamples(), buffer.getNumSamples());

    for (int i = 0; i < numSamples; ++i)
    {
        const auto dry = dryMix.getNextValue();
        const auto wet = 1.0f - dry;

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* out = buffer.getWritePointer (ch);
            const auto* dryData = dryBuffer.getReadPointer (ch);
            out[i] = out[i] * wet + dryData[i] * dry;
        }
    }
}
} // namespace neseq
