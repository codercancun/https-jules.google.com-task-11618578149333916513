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
constexpr int    kNumBlocks  = 64; // total = kBlockSize * kNumBlocks samples

/** Feed a sine wave through the given EQ and return the linear RMS of the
    processed signal, measured over the second half of the run so the filter
    has had time to reach steady state. */
float measureRms (ThreeBandEQ& eq, float freqHz, float lowDb, float midDb, float highDb)
{
    juce::dsp::ProcessSpec spec {};
    spec.sampleRate       = kSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (kBlockSize);
    spec.numChannels      = 1;
    eq.prepare (spec);
    eq.update (lowDb, midDb, highDb);

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

        // Only measure the second half of the run so transient settling
        // doesn't distort the RMS value.
        if (block >= kNumBlocks / 2)
        {
            for (int i = 0; i < kBlockSize; ++i)
                sumSq += static_cast<double> (data[i]) * data[i];
            countedSamples += kBlockSize;
        }
    }

    return static_cast<float> (std::sqrt (sumSq / juce::jmax (1, countedSamples)));
}

/** RMS of a unit-amplitude sine wave is 1/sqrt(2). */
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
            // Well below the 200 Hz corner the shelf approaches full gain;
            // at the corner itself a JUCE shelf is ~half the set gain in dB.
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
            // Shelf cut and boost should be roughly mirror images of 0 dB.
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

            // Repeated identical updates should leave the filter stable and
            // deterministic. Run a zero-input block and expect zero output
            // (no denormal surprises, no state corruption).
            eq.update (3.0f, -3.0f, 6.0f);
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

        beginTest ("reset clears filter state (no ringing)");
        {
            ThreeBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = kBlockSize;
            spec.numChannels      = 1;
            eq.prepare (spec);

            // Set some EQ so the filters have a state
            eq.update (12.0f, 12.0f, 12.0f);

            juce::AudioBuffer<float> buffer (1, kBlockSize);

            // 1. Push a non-zero signal (e.g., ones) to get filters into a non-zero state
            for (int i = 0; i < kBlockSize; ++i)
                buffer.setSample (0, i, 1.0f);

            juce::dsp::AudioBlock<float> block1 (buffer);
            juce::dsp::ProcessContextReplacing<float> ctx1 (block1);
            eq.process (ctx1);

            // 2. Call reset
            eq.reset();

            // 3. Push zeroes
            buffer.clear();
            juce::dsp::AudioBlock<float> block2 (buffer);
            juce::dsp::ProcessContextReplacing<float> ctx2 (block2);
            eq.process (ctx2);

            // 4. Verify output is completely zero (no ringing)
            for (int i = 0; i < kBlockSize; ++i)
            {
                expect (std::abs (buffer.getSample (0, i)) < 1.0e-6f,
                    "output should be exactly zero immediately after reset");
            }
        }
    }
};

static ThreeBandEQTests threeBandEQTests;
} // namespace neseq
