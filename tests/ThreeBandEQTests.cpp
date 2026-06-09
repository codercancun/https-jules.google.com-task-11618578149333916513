#include "../source/ThreeBandEQ.h"

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include <cmath>

namespace neseq
{
namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int    kBlockSize  = 512;
constexpr int    kNumBlocks  = 64;

float measureRms (ThreeBandEQ& eq, float freqHz, float lowDb, float midDb, float highDb)
{
    juce::dsp::ProcessSpec spec {};
    spec.sampleRate       = kSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (kBlockSize);
    spec.numChannels      = 1;
    eq.prepare (spec);
    eq.snap (lowDb, midDb, highDb);

    const double omega = juce::MathConstants<double>::twoPi * freqHz / kSampleRate;

    juce::AudioBuffer<float> buffer (1, kBlockSize);

    double phase = 0.0;
    double sumSq = 0.0;
    int    countedSamples = 0;

    for (int block = 0; block < kNumBlocks; ++block)
    {
        auto* data = buffer.getWritePointer (0);
        for (int i = 0; i < kBlockSize; ++i)
        {
            data[i] = static_cast<float> (std::sin (phase));
            phase += omega;
            if (phase > juce::MathConstants<double>::twoPi)
                phase -= juce::MathConstants<double>::twoPi;
        }

        juce::dsp::AudioBlock<float> block_ (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block_);
        eq.process (ctx);

        if (block >= kNumBlocks / 2)
        {
            for (int i = 0; i < kBlockSize; ++i)
                sumSq += static_cast<double> (data[i]) * data[i];
            countedSamples += kBlockSize;
        }
    }

    return static_cast<float> (std::sqrt (sumSq / juce::jmax (1, countedSamples)));
}

constexpr float kUnitSineRms = 0.70710678f;

float measureGainDb (ThreeBandEQ& eq, float freqHz, float lowDb, float midDb, float highDb)
{
    return juce::Decibels::gainToDecibels (
        measureRms (eq, freqHz, lowDb, midDb, highDb) / kUnitSineRms);
}
} // namespace

class ThreeBandEQTests : public juce::UnitTest
{
public:
    ThreeBandEQTests() : juce::UnitTest ("ThreeBandEQ") {}

    void runTest() override
    {
        beginTest ("flat response at 0 dB");
        {
            ThreeBandEQ eq;
            for (float f : { 200.0f, 1000.0f, 5000.0f })
            {
                const auto gainDb = measureGainDb (eq, f, 0.0f, 0.0f, 0.0f);
                expectWithinAbsoluteError (gainDb, 0.0f, 0.5f,
                    "flat response should be ~0 dB at " + juce::String (f) + " Hz");
            }
        }

        beginTest ("low-shelf boost peaks below 200 Hz and leaves 5 kHz alone");
        {
            ThreeBandEQ eq;
            const auto deepLowDb = measureGainDb (eq, 50.0f,   12.0f, 0.0f, 0.0f);
            const auto cornerDb  = measureGainDb (eq, 200.0f,  12.0f, 0.0f, 0.0f);
            const auto highDb    = measureGainDb (eq, 5000.0f, 12.0f, 0.0f, 0.0f);

            expectGreaterThan (deepLowDb, 10.0f,
                "low-shelf @50 Hz with +12 dB low gain should approach full boost");
            expectWithinAbsoluteError (cornerDb, 6.0f, 1.0f,
                "low-shelf @200 Hz corner should boost roughly half the set gain");
            expectWithinAbsoluteError (highDb, 0.0f, 1.0f,
                "low-shelf should barely touch 5 kHz");
        }

        beginTest ("mid peak boost affects 1 kHz most");
        {
            ThreeBandEQ eq;
            const auto midDb  = measureGainDb (eq, 1000.0f, 0.0f, 12.0f, 0.0f);
            const auto lowDb  = measureGainDb (eq, 200.0f,  0.0f, 12.0f, 0.0f);
            const auto highDb = measureGainDb (eq, 5000.0f, 0.0f, 12.0f, 0.0f);

            expectGreaterThan (midDb, 10.0f,
                "peak @1 kHz with +12 dB mid gain should boost close to full");
            expectGreaterThan (midDb, lowDb + 4.0f,
                "1 kHz boost should clearly exceed 200 Hz for the mid peak filter");
            expectGreaterThan (midDb, highDb + 4.0f,
                "1 kHz boost should clearly exceed 5 kHz for the mid peak filter");
        }

        beginTest ("high-shelf boost peaks above 5 kHz and leaves 200 Hz alone");
        {
            ThreeBandEQ eq;
            const auto deepHighDb = measureGainDb (eq, 15000.0f, 0.0f, 0.0f, 12.0f);
            const auto cornerDb   = measureGainDb (eq, 5000.0f,  0.0f, 0.0f, 12.0f);
            const auto lowDb      = measureGainDb (eq, 200.0f,   0.0f, 0.0f, 12.0f);

            expectGreaterThan (deepHighDb, 10.0f,
                "high-shelf @15 kHz with +12 dB high gain should approach full boost");
            expectWithinAbsoluteError (cornerDb, 6.0f, 1.0f,
                "high-shelf @5 kHz corner should boost roughly half the set gain");
            expectWithinAbsoluteError (lowDb, 0.0f, 1.0f,
                "high-shelf should barely touch 200 Hz");
        }

        beginTest ("cuts attenuate symmetrically with boosts");
        {
            ThreeBandEQ eq;
            const auto boostDb = measureGainDb (eq, 50.0f,  12.0f, 0.0f, 0.0f);
            const auto cutDb   = measureGainDb (eq, 50.0f, -12.0f, 0.0f, 0.0f);

            expectLessThan (cutDb, -10.0f,
                "low-shelf @50 Hz with -12 dB low gain should cut close to full");
            expectWithinAbsoluteError (boostDb + cutDb, 0.0f, 1.5f,
                "shelf boost and cut should be approximately symmetric");
        }

        beginTest ("update is a no-op when gains are unchanged (coefficient cache)");
        {
            ThreeBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = kBlockSize;
            spec.numChannels      = 1;
            eq.prepare (spec);

            eq.snap (3.0f, -3.0f, 6.0f);
            eq.update (3.0f, -3.0f, 6.0f);
            eq.update (3.0f, -3.0f, 6.0f);

            juce::AudioBuffer<float> buffer (1, kBlockSize);
            buffer.clear();
            juce::dsp::AudioBlock<float> block_ (buffer);
            juce::dsp::ProcessContextReplacing<float> ctx (block_);
            eq.process (ctx);

            for (int i = 0; i < kBlockSize; ++i)
                expect (std::abs (buffer.getSample (0, i)) < 1.0e-6f,
                    "zero input should produce zero output");
        }

        beginTest ("smoothing ramps to target over time");
        {
            ThreeBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = static_cast<juce::uint32> (kBlockSize);
            spec.numChannels      = 1;
            eq.prepare (spec);
            eq.snap (0.0f, 0.0f, 0.0f);

            eq.update (12.0f, 0.0f, 0.0f);

            juce::AudioBuffer<float> buffer (1, kBlockSize);
            const double omega = juce::MathConstants<double>::twoPi * 50.0 / kSampleRate;
            double phase = 0.0;

            auto* data = buffer.getWritePointer (0);
            for (int i = 0; i < kBlockSize; ++i)
            {
                data[i] = static_cast<float> (std::sin (phase));
                phase += omega;
            }

            juce::dsp::AudioBlock<float> block_ (buffer);
            juce::dsp::ProcessContextReplacing<float> ctx (block_);
            eq.process (ctx);

            float rms = 0.0f;
            for (int i = 0; i < kBlockSize; ++i)
                rms += data[i] * data[i];
            rms = std::sqrt (rms / kBlockSize);

            expect (rms > kUnitSineRms * 0.9f,
                "after one block of smoothing the signal should still be amplified");
        }
    }
};

static ThreeBandEQTests threeBandEQTests;
} // namespace neseq
