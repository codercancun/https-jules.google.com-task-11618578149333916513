#include "ThreeBandEQ.h"

namespace neseq
{
namespace
{
float dbToGain (float db)
{
    return juce::Decibels::decibelsToGain (db, -96.0f);
}
} // namespace

void ThreeBandEQ::prepare (const juce::dsp::ProcessSpec& spec, float rampSeconds)
{
    sampleRate = spec.sampleRate;
    chain.prepare (spec);

    const double ramp = static_cast<double> (juce::jmax (0.0f, rampSeconds));

    smoothedLow .reset (sampleRate, ramp);
    smoothedMid .reset (sampleRate, ramp);
    smoothedHigh.reset (sampleRate, ramp);

    // Force a rebuild on the next call to update.
    currentLowDb  = std::numeric_limits<float>::infinity();
    currentMidDb  = std::numeric_limits<float>::infinity();
    currentHighDb = std::numeric_limits<float>::infinity();

    update (0.0f, 0.0f, 0.0f);

    // Skip the ramp on the initial state so we start at 0 dB immediately.
    smoothedLow .skip (static_cast<int> (sampleRate * ramp));
    smoothedMid .skip (static_cast<int> (sampleRate * ramp));
    smoothedHigh.skip (static_cast<int> (sampleRate * ramp));
}

void ThreeBandEQ::reset()
{
    chain.reset();
}

void ThreeBandEQ::update (float lowGainDb, float midGainDb, float highGainDb)
{
    if (std::abs (lowGainDb - currentLowDb) > kGainEpsilonDb)
    {
        smoothedLow.setTargetValue (lowGainDb);
        currentLowDb = lowGainDb;
    }

    if (std::abs (midGainDb - currentMidDb) > kGainEpsilonDb)
    {
        smoothedMid.setTargetValue (midGainDb);
        currentMidDb = midGainDb;
    }

    if (std::abs (highGainDb - currentHighDb) > kGainEpsilonDb)
    {
        smoothedHigh.setTargetValue (highGainDb);
        currentHighDb = highGainDb;
    }

    // If not smoothing, rebuild coefficients at the current target immediately.
    if (! isSmoothing())
    {
        updateBand (Low,  smoothedLow.getCurrentValue());
        updateBand (Mid,  smoothedMid.getCurrentValue());
        updateBand (High, smoothedHigh.getCurrentValue());
    }
}

void ThreeBandEQ::advanceSmoothing()
{
    const float lowDb  = smoothedLow.getNextValue();
    const float midDb  = smoothedMid.getNextValue();
    const float highDb = smoothedHigh.getNextValue();

    updateBand (Low,  lowDb);
    updateBand (Mid,  midDb);
    updateBand (High, highDb);
}

float ThreeBandEQ::processSingleSample (float sample)
{
    auto& lowFilter  = chain.get<Low>();
    auto& midFilter  = chain.get<Mid>();
    auto& highFilter = chain.get<High>();

    sample = lowFilter.processSample (sample);
    sample = midFilter.processSample (sample);
    sample = highFilter.processSample (sample);
    return sample;
}

void ThreeBandEQ::updateBand (BandIndex band, float gainDb)
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
