#include "BypassCrossfader.h"

namespace neseq
{
void BypassCrossfader::prepare (double sampleRateIn,
                                int    numChannels,
                                int    maximumBlockSize,
                                double rampTimeSeconds)
{
    sampleRate  = sampleRateIn;
    rampSeconds = juce::jmax (0.0, rampTimeSeconds);

    dryBuffer.setSize (juce::jmax (1, numChannels),
                       juce::jmax (1, maximumBlockSize),
                       false,  // keepExistingContent
                       true,   // clearExtraSpace
                       true);  // avoidReallocating
    dryBuffer.clear();

    const float currentTarget = dryMix.getTargetValue();
    dryMix.reset (sampleRate, rampSeconds);
    dryMix.setCurrentAndTargetValue (currentTarget);
}

void BypassCrossfader::setBypassed (bool bypassed) noexcept
{
    dryMix.setTargetValue (bypassed ? 1.0f : 0.0f);
}

bool BypassCrossfader::isFullyBypassed() const noexcept
{
    return ! dryMix.isSmoothing() && dryMix.getTargetValue() >= 0.5f;
}

bool BypassCrossfader::isFullyActive() const noexcept
{
    return ! dryMix.isSmoothing() && dryMix.getTargetValue() < 0.5f;
}

bool BypassCrossfader::isSmoothing() const noexcept
{
    return dryMix.isSmoothing();
}

float BypassCrossfader::getCurrentDryMix() const noexcept
{
    return dryMix.getCurrentValue();
}

bool BypassCrossfader::shouldRunEffect() const noexcept
{
    return ! isFullyBypassed();
}

bool BypassCrossfader::shouldCaptureDry() const noexcept
{
    return dryMix.isSmoothing() || dryMix.getTargetValue() > 0.0f;
}

void BypassCrossfader::captureDry (const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numChannels = juce::jmin (buffer.getNumChannels(), dryBuffer.getNumChannels());
    const auto numSamples  = juce::jmin (buffer.getNumSamples(),  dryBuffer.getNumSamples());

    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);
}

void BypassCrossfader::mix (juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples  = buffer.getNumSamples();

    if (numSamples <= 0 || numChannels <= 0)
        return;

    // Fast path: no crossfade in progress.
    if (! dryMix.isSmoothing())
    {
        const auto mix = dryMix.getCurrentValue();
        if (mix <= 0.0f)
            return; // fully wet; buffer already holds the wet signal

        if (mix >= 1.0f)
        {
            // Fully dry: copy the dry buffer back over the wet one.
            const auto dryChannels = juce::jmin (numChannels, dryBuffer.getNumChannels());
            for (int ch = 0; ch < dryChannels; ++ch)
                buffer.copyFrom (ch, 0, dryBuffer, ch, 0, numSamples);
            return;
        }
    }

    const auto dryChannels = juce::jmin (numChannels, dryBuffer.getNumChannels());

    // Per-sample linear crossfade. Reading ``getNextValue`` per sample
    // guarantees both channels see exactly the same ramp.
    for (int i = 0; i < numSamples; ++i)
    {
        const float dryGain = dryMix.getNextValue();
        const float wetGain = 1.0f - dryGain;

        for (int ch = 0; ch < dryChannels; ++ch)
        {
            const auto wet = buffer.getSample (ch, i);
            const auto dry = dryBuffer.getSample (ch, i);
            buffer.setSample (ch, i, wet * wetGain + dry * dryGain);
        }
    }
}
} // namespace neseq
