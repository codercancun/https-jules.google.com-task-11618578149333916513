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
    {
        DBG ("ThreeBandEQ::prepare called with invalid sample rate: "
             + juce::String (spec.sampleRate));
        return;
    }

    sampleRate = spec.sampleRate;
    chain.prepare (spec);

    // Force a rebuild on the next call to ``update`` by nudging the cached
    // gains off the default so the epsilon check triggers.
    currentLowDb  = std::numeric_limits<float>::infinity();
    currentMidDb  = std::numeric_limits<float>::infinity();
    currentHighDb = std::numeric_limits<float>::infinity();

    update (0.0f, 0.0f, 0.0f);
}

void ThreeBandEQ::reset()
{
    chain.reset();
}

void ThreeBandEQ::update (float lowGainDb, float midGainDb, float highGainDb)
{
    if (std::abs (lowGainDb - currentLowDb) > kGainEpsilonDb)
    {
        updateBand (Low, lowGainDb);
        currentLowDb = lowGainDb;
    }

    if (std::abs (midGainDb - currentMidDb) > kGainEpsilonDb)
    {
        updateBand (Mid, midGainDb);
        currentMidDb = midGainDb;
    }

    if (std::abs (highGainDb - currentHighDb) > kGainEpsilonDb)
    {
        updateBand (High, highGainDb);
        currentHighDb = highGainDb;
    }
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
            break;
        case Mid:
            newCoefficients = Coefficients::makePeakFilter (sampleRate,
                                                            kMidFreqHz,
                                                            kMidQ,
                                                            linearGain);
            break;
        case High:
            newCoefficients = Coefficients::makeHighShelf (sampleRate,
                                                           kHighFreqHz,
                                                           0.707f,
                                                           linearGain);
            break;
        default:
            jassertfalse;
            return;
    }

    if (newCoefficients == nullptr)
    {
        DBG ("ThreeBandEQ::updateBand – coefficient creation returned null for band "
             + juce::String (static_cast<int> (band))
             + " (sampleRate=" + juce::String (sampleRate)
             + ", gainDb=" + juce::String (gainDb, 2) + ")");
        jassertfalse;
        return;
    }

    switch (band)
    {
        case Low:  chain.get<Low>().coefficients  = newCoefficients; break;
        case Mid:  chain.get<Mid>().coefficients  = newCoefficients; break;
        case High: chain.get<High>().coefficients = newCoefficients; break;
        default:   break;
    }
}
} // namespace neseq
