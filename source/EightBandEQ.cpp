#include "EightBandEQ.h"

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

void EightBandEQ::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    for (auto& f : filters)
        f.prepare (spec);

    // Force a rebuild on the next call to ``update``.
    currentDb.fill (std::numeric_limits<float>::infinity());

    std::array<float, kNumBands> zeros {};
    update (zeros);
}

void EightBandEQ::reset()
{
    for (auto& f : filters)
        f.reset();
}

void EightBandEQ::update (const std::array<float, kNumBands>& gainsDb)
{
    for (int i = 0; i < kNumBands; ++i)
    {
        if (std::abs (gainsDb[static_cast<size_t> (i)] - currentDb[static_cast<size_t> (i)]) > kGainEpsilonDb)
        {
            updateBand (i, gainsDb[static_cast<size_t> (i)]);
            currentDb[static_cast<size_t> (i)] = gainsDb[static_cast<size_t> (i)];
        }
    }
}

EightBandEQ::FilterType EightBandEQ::typeForBand (int band) const
{
    if (band == 0)                return LowShelf;
    if (band == kNumBands - 1)    return HighShelf;
    return Peak;
}

void EightBandEQ::updateBand (int band, float gainDb)
{
    const auto linearGain = dbToGain (gainDb);
    const auto freq = kFrequencies[static_cast<size_t> (band)];

    juce::ReferenceCountedObjectPtr<Coefficients> newCoefficients;

    switch (typeForBand (band))
    {
        case LowShelf:
            newCoefficients = Coefficients::makeLowShelf (sampleRate, freq,
                                                          0.707f, linearGain);
            break;
        case Peak:
            newCoefficients = Coefficients::makePeakFilter (sampleRate, freq,
                                                             kPeakQ, linearGain);
            break;
        case HighShelf:
            newCoefficients = Coefficients::makeHighShelf (sampleRate, freq,
                                                           0.707f, linearGain);
            break;
    }

    filters[static_cast<size_t> (band)].coefficients = newCoefficients;
}
} // namespace neseq
