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

    juce::ReferenceCountedObjectPtr<Coefficients> newCoefficients;

    switch (band)
    {
        case Low:
            newCoefficients = Coefficients::makeLowShelf (sampleRate,
                                                          kLowFreqHz,
                                                          0.707f,
                                                          linearGain);
            chain.get<Low>().coefficients = newCoefficients;
            break;
        case Mid:
            newCoefficients = Coefficients::makePeakFilter (sampleRate,
                                                            kMidFreqHz,
                                                            kMidQ,
                                                            linearGain);
            chain.get<Mid>().coefficients = newCoefficients;
            break;
        case High:
            newCoefficients = Coefficients::makeHighShelf (sampleRate,
                                                           kHighFreqHz,
                                                           0.707f,
                                                           linearGain);
            chain.get<High>().coefficients = newCoefficients;
            break;
    }
}
} // namespace neseq
