#include "ThreeBandEQ.h"

namespace neseq
{
namespace
{
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

    smoothedLow .reset (sampleRate, smoothingTimeSec);
    smoothedMid .reset (sampleRate, smoothingTimeSec);
    smoothedHigh.reset (sampleRate, smoothingTimeSec);

    snap (0.0f, 0.0f, 0.0f);
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

    currentLowDb  = std::numeric_limits<float>::infinity();
    currentMidDb  = std::numeric_limits<float>::infinity();
    currentHighDb = std::numeric_limits<float>::infinity();

    advanceSmoothersAndRebuild (0);
}

void ThreeBandEQ::advanceSmoothersAndRebuild (int numSamples)
{
    const float lowDb  = smoothedLow .skip (numSamples);
    const float midDb  = smoothedMid .skip (numSamples);
    const float highDb = smoothedHigh.skip (numSamples);

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
