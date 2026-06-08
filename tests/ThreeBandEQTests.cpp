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

        // ---- Additional edge-case tests ----

        beginTest ("reset clears filter state to zero");
        {
            ThreeBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = kBlockSize;
            spec.numChannels      = 1;
            eq.prepare (spec);
            eq.update (12.0f, 6.0f, 9.0f);

            // Feed signal to build up internal state.
            juce::AudioBuffer<float> buffer (1, kBlockSize);
            for (int b = 0; b < 8; ++b)
            {
                auto* data = buffer.getWritePointer (0);
                for (int i = 0; i < kBlockSize; ++i)
                    data[i] = static_cast<float> (std::sin (
                        juce::MathConstants<double>::twoPi * 200.0 * (b * kBlockSize + i) / kSampleRate));

                juce::dsp::AudioBlock<float> block_ (buffer);
                juce::dsp::ProcessContextReplacing<float> ctx (block_);
                eq.process (ctx);
            }

            // Reset and process zero input — output should be zero.
            eq.reset();
            buffer.clear();
            {
                juce::dsp::AudioBlock<float> block_ (buffer);
                juce::dsp::ProcessContextReplacing<float> ctx (block_);
                eq.process (ctx);
            }

            for (int i = 0; i < kBlockSize; ++i)
                expect (std::abs (buffer.getSample (0, i)) < 1.0e-6f,
                    "after reset, zero input should produce zero output");
        }

        beginTest ("works at multiple sample rates (22050, 44100, 96000)");
        {
            for (double sr : { 22050.0, 44100.0, 96000.0 })
            {
                ThreeBandEQ eq;
                juce::dsp::ProcessSpec spec {};
                spec.sampleRate       = sr;
                spec.maximumBlockSize = kBlockSize;
                spec.numChannels      = 1;
                eq.prepare (spec);
                eq.update (6.0f, 0.0f, 0.0f);

                const double omega = juce::MathConstants<double>::twoPi * 50.0 / sr;
                juce::AudioBuffer<float> buffer (1, kBlockSize);
                double phase = 0.0;

                float lastRms = 0.0f;
                for (int b = 0; b < kNumBlocks; ++b)
                {
                    auto* data = buffer.getWritePointer (0);
                    for (int i = 0; i < kBlockSize; ++i)
                    {
                        data[i] = static_cast<float> (std::sin (phase));
                        phase += omega;
                    }
                    juce::dsp::AudioBlock<float> block_ (buffer);
                    juce::dsp::ProcessContextReplacing<float> ctx (block_);
                    eq.process (ctx);

                    if (b >= kNumBlocks / 2)
                    {
                        double sumSq = 0.0;
                        for (int i = 0; i < kBlockSize; ++i)
                            sumSq += static_cast<double> (data[i]) * data[i];
                        lastRms = static_cast<float> (std::sqrt (sumSq / kBlockSize));
                    }
                }

                expectGreaterThan (lastRms, kUnitSineRms * 1.3f,
                    "low-shelf boost should audibly boost 50 Hz at sample rate "
                        + juce::String (sr));
            }
        }

        beginTest ("extreme gains: ±15 dB produces expected magnitude");
        {
            ThreeBandEQ eq;
            const auto maxBoostDb = measureGainDb (eq, 50.0f, 15.0f, 0.0f, 0.0f);
            expectGreaterThan (maxBoostDb, 13.0f,
                "low-shelf @50 Hz with max +15 dB should approach full boost");

            ThreeBandEQ eq2;
            const auto maxCutDb = measureGainDb (eq2, 50.0f, -15.0f, 0.0f, 0.0f);
            expectLessThan (maxCutDb, -13.0f,
                "low-shelf @50 Hz with max -15 dB should cut heavily");
        }

        beginTest ("all three bands active simultaneously");
        {
            ThreeBandEQ eq;
            // Low boost, mid flat, high cut.
            const auto lowDb  = measureGainDb (eq, 50.0f,   10.0f, 0.0f, -10.0f);
            const auto midDb  = measureGainDb (eq, 1000.0f, 10.0f, 0.0f, -10.0f);
            const auto highDb = measureGainDb (eq, 15000.0f, 10.0f, 0.0f, -10.0f);

            expectGreaterThan (lowDb, 8.0f,
                "50 Hz should be boosted when low=+10 dB even with high=-10 dB");
            expectLessThan (highDb, -6.0f,
                "15 kHz should be cut when high=-10 dB even with low=+10 dB");
            // 1 kHz should be only modestly affected — the low shelf tail and
            // high shelf tail partially overlap there.
            expectWithinAbsoluteError (midDb, 0.0f, 4.0f,
                "1 kHz should be near flat with low=+10, mid=0, high=-10");
        }

        beginTest ("re-prepare resets and allows processing at a new sample rate");
        {
            ThreeBandEQ eq;

            // First prepare at 44100.
            auto gainA = measureGainDb (eq, 200.0f, 12.0f, 0.0f, 0.0f);
            expectGreaterThan (gainA, 4.0f,
                "initial prepare: low-shelf should boost at 200 Hz");

            // Re-prepare at 96000 and measure again.
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = 96000.0;
            spec.maximumBlockSize = kBlockSize;
            spec.numChannels      = 1;
            eq.prepare (spec);

            // measureRms internally calls prepare with kSampleRate (48000), so
            // we manually run at 96000 here.
            eq.update (12.0f, 0.0f, 0.0f);
            const double omega = juce::MathConstants<double>::twoPi * 200.0 / 96000.0;
            juce::AudioBuffer<float> buffer (1, kBlockSize);
            double phase = 0.0;
            float lastRms = 0.0f;

            for (int b = 0; b < kNumBlocks; ++b)
            {
                auto* data = buffer.getWritePointer (0);
                for (int i = 0; i < kBlockSize; ++i)
                {
                    data[i] = static_cast<float> (std::sin (phase));
                    phase += omega;
                }
                juce::dsp::AudioBlock<float> block_ (buffer);
                juce::dsp::ProcessContextReplacing<float> ctx (block_);
                eq.process (ctx);

                if (b >= kNumBlocks / 2)
                {
                    double sumSq = 0.0;
                    for (int i = 0; i < kBlockSize; ++i)
                        sumSq += static_cast<double> (data[i]) * data[i];
                    lastRms = static_cast<float> (std::sqrt (sumSq / kBlockSize));
                }
            }

            const auto gainB = juce::Decibels::gainToDecibels (lastRms / kUnitSineRms);
            expectGreaterThan (gainB, 4.0f,
                "after re-prepare at 96 kHz, low-shelf should still boost at 200 Hz");
        }

        beginTest ("mid cut attenuates 1 kHz and leaves 50 Hz alone");
        {
            ThreeBandEQ eq;
            const auto midDb  = measureGainDb (eq, 1000.0f, 0.0f, -12.0f, 0.0f);
            const auto lowDb  = measureGainDb (eq, 50.0f,   0.0f, -12.0f, 0.0f);

            expectLessThan (midDb, -10.0f,
                "peak @1 kHz with -12 dB mid gain should cut close to full");
            expectWithinAbsoluteError (lowDb, 0.0f, 1.5f,
                "peak mid cut should not significantly affect 50 Hz");
        }
    }
};

static ThreeBandEQTests threeBandEQTests;
} // namespace neseq
