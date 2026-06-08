#include "BypassCrossfader.h"

namespace neseq
{
void BypassCrossfader::prepare (double sampleRate, int numChannels,
                                int maxBlockSize, double rampSeconds)
{
    dryBuffer.setSize (numChannels, maxBlockSize);
    dryBuffer.clear();

    wetGain.reset (sampleRate, juce::jmax (0.0, rampSeconds));
    wetGain.setCurrentAndTargetValue (isBypassed ? 0.0f : 1.0f);
}

void BypassCrossfader::setBypassed (bool bypassed) noexcept
{
    if (bypassed == isBypassed)
        return;

    isBypassed = bypassed;
    wetGain.setTargetValue (bypassed ? 0.0f : 1.0f);
}

void BypassCrossfader::captureDry (const juce::AudioBuffer<float>& buffer)
{
    const auto numChannels = juce::jmin (buffer.getNumChannels(),
                                         dryBuffer.getNumChannels());
    const auto numSamples  = juce::jmin (buffer.getNumSamples(),
                                         dryBuffer.getNumSamples());

    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);
}

void BypassCrossfader::mix (juce::AudioBuffer<float>& wetBuffer)
{
    if (! isRamping())
    {
        if (isBypassed)
        {
            const auto numChannels = juce::jmin (wetBuffer.getNumChannels(),
                                                 dryBuffer.getNumChannels());
            const auto numSamples  = juce::jmin (wetBuffer.getNumSamples(),
                                                 dryBuffer.getNumSamples());
            for (int ch = 0; ch < numChannels; ++ch)
                wetBuffer.copyFrom (ch, 0, dryBuffer, ch, 0, numSamples);
        }
        return;
    }

    const auto numChannels = juce::jmin (wetBuffer.getNumChannels(),
                                         dryBuffer.getNumChannels());
    const auto numSamples  = juce::jmin (wetBuffer.getNumSamples(),
                                         dryBuffer.getNumSamples());

    for (int i = 0; i < numSamples; ++i)
    {
        const float wet = wetGain.getNextValue();
        const float dry = 1.0f - wet;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float wetSample = wetBuffer.getSample (ch, i);
            const float drySample = dryBuffer.getSample (ch, i);
            wetBuffer.setSample (ch, i, wetSample * wet + drySample * dry);
        }
    }
}
} // namespace neseq
