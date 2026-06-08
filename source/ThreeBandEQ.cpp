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
    auto updateIfChanged = [&] (BandIndex band, float newDb, float& cached)
    {
        if (std::abs (newDb - cached) > kGainEpsilonDb)
        {
            updateBand (band, newDb);
            cached = newDb;
        }
    };

    updateIfChanged (Low,  lowGainDb,  currentLowDb);
    updateIfChanged (Mid,  midGainDb,  currentMidDb);
    updateIfChanged (High, highGainDb, currentHighDb);
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
