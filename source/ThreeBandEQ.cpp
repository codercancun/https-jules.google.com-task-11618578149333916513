#include "ThreeBandEQ.h"

namespace neseq
{
namespace
{
// A gain change smaller than this (in dB) is treated as a no-op so we skip the
// coefficient rebuild. Keeps things lock free and allocation free.
constexpr float kGainEpsilonDb = 1.0e-3f;

float dbToGain (float db)
{
    return juce::Decibels::decibelsToGain (db, -96.0f);
}
} // namespace

void ThreeBandEQ::prepare (const juce::dsp::ProcessSpec& spec)
{
    jassert (spec.sampleRate > 0.0);

    if (spec.sampleRate <= 0.0)
        return;

    sampleRate = spec.sampleRate;
    chain.prepare (spec);

    const double rampSeconds = juce::jmax (0.0f, smoothingTimeSec);

    const float targetLow  = smoothedLow .getTargetValue();
    const float targetMid  = smoothedMid .getTargetValue();
    const float targetHigh = smoothedHigh.getTargetValue();

    smoothedLow .reset (sampleRate, rampSeconds);
    smoothedMid .reset (sampleRate, rampSeconds);
    smoothedHigh.reset (sampleRate, rampSeconds);

    // Snap the smoothers to whatever target the host last requested so we
    // don't sweep audibly on the first block after prepare().
    smoothedLow .setCurrentAndTargetValue (targetLow);
    smoothedMid .setCurrentAndTargetValue (targetMid);
    smoothedHigh.setCurrentAndTargetValue (targetHigh);

    // Force a coefficient rebuild against the current (post-reset) gains.
    currentLowDb  = std::numeric_limits<float>::infinity();
    currentMidDb  = std::numeric_limits<float>::infinity();
    currentHighDb = std::numeric_limits<float>::infinity();
    advanceSmoothersAndRebuild (0);
}

void ThreeBandEQ::reset()
{
    chain.reset();
}

void ThreeBandEQ::update (float lowGainDb, float midGainDb, float highGainDb)
{
    smoothedLow .setTargetValue (lowGainDb);
    smoothedMid .setTargetValue (midGainDb);
    smoothedHigh.setTargetValue (highGainDb);
}

void ThreeBandEQ::snap (float lowGainDb, float midGainDb, float highGainDb)
{
    smoothedLow .setCurrentAndTargetValue (lowGainDb);
    smoothedMid .setCurrentAndTargetValue (midGainDb);
    smoothedHigh.setCurrentAndTargetValue (highGainDb);
    advanceSmoothersAndRebuild (0);
}

void ThreeBandEQ::setSmoothingTime (float seconds) noexcept
{
    smoothingTimeSec = juce::jmax (0.0f, seconds);
}

void ThreeBandEQ::advanceSmoothersAndRebuild (int numSamples)
{
    const float lowDb  = numSamples > 0 ? smoothedLow .skip (numSamples)
                                        : smoothedLow .getCurrentValue();
    const float midDb  = numSamples > 0 ? smoothedMid .skip (numSamples)
                                        : smoothedMid .getCurrentValue();
    const float highDb = numSamples > 0 ? smoothedHigh.skip (numSamples)
                                        : smoothedHigh.getCurrentValue();

    if (std::abs (lowDb - currentLowDb) > kGainEpsilonDb)
    {
        rebuildBand (Low, lowDb);
        currentLowDb = lowDb;
    }

    if (std::abs (midDb - currentMidDb) > kGainEpsilonDb)
    {
        rebuildBand (Mid, midDb);
        currentMidDb = midDb;
    }

    if (std::abs (highDb - currentHighDb) > kGainEpsilonDb)
    {
        rebuildBand (High, highDb);
        currentHighDb = highDb;
    }
}

void ThreeBandEQ::rebuildBand (BandIndex band, float gainDb)
{
    const auto linearGain = dbToGain (gainDb);

    // Write directly into the existing Coefficients object to avoid heap
    // allocation on the audio thread.
    switch (band)
    {
        case Low:
            *chain.get<Low>().coefficients =
                juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf (sampleRate,
                                                                        kLowFreqHz,
                                                                        0.707f,
                                                                        linearGain);
            break;
        case Mid:
            *chain.get<Mid>().coefficients =
                juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (sampleRate,
                                                                          kMidFreqHz,
                                                                          kMidQ,
                                                                          linearGain);
            break;
        case High:
            *chain.get<High>().coefficients =
                juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf (sampleRate,
                                                                         kHighFreqHz,
                                                                         0.707f,
                                                                         linearGain);
            break;
        default:
            jassertfalse;
            break;
    }
}
} // namespace neseq
