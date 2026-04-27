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
    blockSize  = static_cast<int> (spec.maximumBlockSize);
    chain.prepare (spec);

    for (int i = 0; i < NumBands; ++i)
    {
        smoothedGainDb[i].reset (sampleRate, kSmoothingTimeSec);
        smoothedGainDb[i].setCurrentAndTargetValue (0.0f);
        lastAppliedDb[i] = std::numeric_limits<float>::infinity();
    }

    update (0.0f, 0.0f, 0.0f);

    // Force an immediate coefficient build at 0 dB.
    for (int i = 0; i < NumBands; ++i)
    {
        smoothedGainDb[i].setCurrentAndTargetValue (0.0f);
        updateBand (static_cast<BandIndex> (i), 0.0f);
        lastAppliedDb[i] = 0.0f;
    }
}

void ThreeBandEQ::reset()
{
    chain.reset();
}

void ThreeBandEQ::update (float lowGainDb, float midGainDb, float highGainDb)
{
    smoothedGainDb[Low].setTargetValue (lowGainDb);
    smoothedGainDb[Mid].setTargetValue (midGainDb);
    smoothedGainDb[High].setTargetValue (highGainDb);
}

void ThreeBandEQ::process (juce::dsp::AudioBlock<float>& block)
{
    const auto numSamples = static_cast<int> (block.getNumSamples());

    if (numSamples == 0)
        return;

    // If no smoother is ramping, process the whole block at once.
    if (! isSmoothing())
    {
        // Update coefficients once in case setTargetValue was just called.
        for (int i = 0; i < NumBands; ++i)
        {
            const float db = smoothedGainDb[i].getTargetValue();
            if (std::abs (db - lastAppliedDb[i]) > kGainEpsilonDb)
            {
                updateBand (static_cast<BandIndex> (i), db);
                lastAppliedDb[i] = db;
            }
        }

        auto ctx = juce::dsp::ProcessContextReplacing<float> (block);
        chain.process (ctx);
        return;
    }

    // Process in sub-blocks when smoothing is active.
    // Use a moderate sub-block size to balance smoothness vs. CPU.
    constexpr int kSubBlockSize = 32;
    int samplesRemaining = numSamples;
    int startSample = 0;

    while (samplesRemaining > 0)
    {
        const int chunkSize = juce::jmin (kSubBlockSize, samplesRemaining);

        // Advance smoothers and update coefficients.
        for (int i = 0; i < NumBands; ++i)
        {
            smoothedGainDb[i].skip (chunkSize);
            const float db = smoothedGainDb[i].getCurrentValue();
            if (std::abs (db - lastAppliedDb[i]) > kGainEpsilonDb)
            {
                updateBand (static_cast<BandIndex> (i), db);
                lastAppliedDb[i] = db;
            }
        }

        auto subBlock = block.getSubBlock (static_cast<size_t> (startSample),
                                           static_cast<size_t> (chunkSize));
        auto ctx = juce::dsp::ProcessContextReplacing<float> (subBlock);
        chain.process (ctx);

        startSample += chunkSize;
        samplesRemaining -= chunkSize;
    }
}

float ThreeBandEQ::getSmoothedGainDb (int bandIndex) const
{
    if (bandIndex >= 0 && bandIndex < NumBands)
        return smoothedGainDb[bandIndex].getCurrentValue();
    return 0.0f;
}

bool ThreeBandEQ::isSmoothing() const
{
    return smoothedGainDb[Low].isSmoothing()
        || smoothedGainDb[Mid].isSmoothing()
        || smoothedGainDb[High].isSmoothing();
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
        case NumBands:
            break;
    }
}
} // namespace neseq
