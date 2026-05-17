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
    }
}
} // namespace neseq
