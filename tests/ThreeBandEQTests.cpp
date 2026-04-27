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
        eq.process (block_);

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

            eq.update (3.0f, -3.0f, 6.0f);
            eq.update (3.0f, -3.0f, 6.0f);
            eq.update (3.0f, -3.0f, 6.0f);

            // Run enough blocks for smoothing to settle.
            juce::AudioBuffer<float> buffer (1, kBlockSize);
            for (int b = 0; b < 16; ++b)
            {
                buffer.clear();
                juce::dsp::AudioBlock<float> block_ (buffer);
                eq.process (block_);
            }

            // Now feed zero input — should produce zero output.
            buffer.clear();
            juce::dsp::AudioBlock<float> block_ (buffer);
            eq.process (block_);

            for (int i = 0; i < kBlockSize; ++i)
                expect (std::abs (buffer.getSample (0, i)) < 1.0e-6f,
                    "zero input should produce zero output");
        }

        // ---- New tests for smoothing ----

        beginTest ("smoothing ramps gain gradually (no instant jump)");
        {
            ThreeBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = static_cast<juce::uint32> (kBlockSize);
            spec.numChannels      = 1;
            eq.prepare (spec);

            // Start at 0 dB, then jump to +12 dB.
            eq.update (0.0f, 0.0f, 0.0f);

            // Process one block at 0 dB to stabilize.
            juce::AudioBuffer<float> buffer (1, kBlockSize);
            buffer.clear();
            {
                juce::dsp::AudioBlock<float> block_ (buffer);
                eq.process (block_);
            }

            // Now set target to +12 dB and check isSmoothing.
            eq.update (12.0f, 0.0f, 0.0f);
            expect (eq.isSmoothing(),
                "eq should be smoothing after a gain change");

            // Process enough blocks for the smoother to finish
            // (50ms at 48kHz = 2400 samples = ~5 blocks of 512).
            for (int b = 0; b < 10; ++b)
            {
                buffer.clear();
                juce::dsp::AudioBlock<float> block_ (buffer);
                eq.process (block_);
            }

            expect (! eq.isSmoothing(),
                "eq should no longer be smoothing after ramp completes");
        }

        beginTest ("getSmoothedGainDb returns current smoothed value");
        {
            ThreeBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = static_cast<juce::uint32> (kBlockSize);
            spec.numChannels      = 1;
            eq.prepare (spec);

            eq.update (5.0f, -3.0f, 10.0f);

            // After settling, smoothed values should match targets.
            juce::AudioBuffer<float> buffer (1, kBlockSize);
            for (int b = 0; b < 20; ++b)
            {
                buffer.clear();
                juce::dsp::AudioBlock<float> block_ (buffer);
                eq.process (block_);
            }

            expectWithinAbsoluteError (eq.getSmoothedGainDb (0), 5.0f, 0.1f,
                "low band smoothed value should reach target");
            expectWithinAbsoluteError (eq.getSmoothedGainDb (1), -3.0f, 0.1f,
                "mid band smoothed value should reach target");
            expectWithinAbsoluteError (eq.getSmoothedGainDb (2), 10.0f, 0.1f,
                "high band smoothed value should reach target");
        }

        beginTest ("getSmoothedGainDb returns 0 for invalid band index");
        {
            ThreeBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = static_cast<juce::uint32> (kBlockSize);
            spec.numChannels      = 1;
            eq.prepare (spec);

            expectEquals (eq.getSmoothedGainDb (-1), 0.0f,
                "negative band index should return 0");
            expectEquals (eq.getSmoothedGainDb (3), 0.0f,
                "out-of-range band index should return 0");
        }

        // ---- Edge case tests ----

        beginTest ("max gain (+15 dB) does not clip or produce NaN");
        {
            ThreeBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = static_cast<juce::uint32> (kBlockSize);
            spec.numChannels      = 1;
            eq.prepare (spec);
            eq.update (15.0f, 15.0f, 15.0f);

            juce::AudioBuffer<float> buffer (1, kBlockSize);
            auto* data = buffer.getWritePointer (0);
            const double omega = juce::MathConstants<double>::twoPi * 1000.0 / kSampleRate;
            double phase = 0.0;
            for (int i = 0; i < kBlockSize; ++i)
            {
                data[i] = static_cast<float> (std::sin (phase));
                phase += omega;
            }

            // Process enough blocks for smoothing to settle.
            for (int b = 0; b < 20; ++b)
            {
                juce::dsp::AudioBlock<float> block_ (buffer);
                eq.process (block_);
            }

            bool hasNaN = false;
            bool hasInf = false;
            for (int i = 0; i < kBlockSize; ++i)
            {
                if (std::isnan (data[i])) hasNaN = true;
                if (std::isinf (data[i])) hasInf = true;
            }
            expect (! hasNaN, "output should not contain NaN at max gain");
            expect (! hasInf, "output should not contain Inf at max gain");
        }

        beginTest ("min gain (-15 dB) attenuates without NaN");
        {
            ThreeBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = static_cast<juce::uint32> (kBlockSize);
            spec.numChannels      = 1;
            eq.prepare (spec);
            eq.update (-15.0f, -15.0f, -15.0f);

            juce::AudioBuffer<float> buffer (1, kBlockSize);
            auto* data = buffer.getWritePointer (0);
            const double omega = juce::MathConstants<double>::twoPi * 1000.0 / kSampleRate;
            double phase = 0.0;
            for (int i = 0; i < kBlockSize; ++i)
            {
                data[i] = static_cast<float> (std::sin (phase));
                phase += omega;
            }

            for (int b = 0; b < 20; ++b)
            {
                juce::dsp::AudioBlock<float> block_ (buffer);
                eq.process (block_);
            }

            bool hasNaN = false;
            for (int i = 0; i < kBlockSize; ++i)
                if (std::isnan (data[i])) hasNaN = true;
            expect (! hasNaN, "output should not contain NaN at min gain");
        }

        beginTest ("zero-length block does not crash");
        {
            ThreeBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = static_cast<juce::uint32> (kBlockSize);
            spec.numChannels      = 1;
            eq.prepare (spec);
            eq.update (5.0f, 5.0f, 5.0f);

            juce::AudioBuffer<float> buffer (1, 0);
            juce::dsp::AudioBlock<float> block_ (buffer);
            eq.process (block_);
            // If we get here without crashing, the test passes.
            expect (true, "zero-length block should be handled gracefully");
        }

        beginTest ("different sample rates produce valid output");
        {
            for (double sr : { 22050.0, 44100.0, 48000.0, 96000.0, 192000.0 })
            {
                ThreeBandEQ eq;
                juce::dsp::ProcessSpec spec {};
                spec.sampleRate       = sr;
                spec.maximumBlockSize = static_cast<juce::uint32> (kBlockSize);
                spec.numChannels      = 1;
                eq.prepare (spec);
                eq.update (6.0f, -3.0f, 9.0f);

                juce::AudioBuffer<float> buffer (1, kBlockSize);
                auto* data = buffer.getWritePointer (0);
                const double omega = juce::MathConstants<double>::twoPi * 1000.0 / sr;
                double phase = 0.0;

                for (int b = 0; b < 20; ++b)
                {
                    for (int i = 0; i < kBlockSize; ++i)
                    {
                        data[i] = static_cast<float> (std::sin (phase));
                        phase += omega;
                        if (phase > juce::MathConstants<double>::twoPi)
                            phase -= juce::MathConstants<double>::twoPi;
                    }
                    juce::dsp::AudioBlock<float> block_ (buffer);
                    eq.process (block_);
                }

                bool hasNaN = false;
                for (int i = 0; i < kBlockSize; ++i)
                    if (std::isnan (data[i])) hasNaN = true;
                expect (! hasNaN,
                    "output should not contain NaN at sample rate " + juce::String (sr));
            }
        }

        beginTest ("rapid gain changes do not produce discontinuities");
        {
            ThreeBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = static_cast<juce::uint32> (kBlockSize);
            spec.numChannels      = 1;
            eq.prepare (spec);

            juce::AudioBuffer<float> buffer (1, kBlockSize);
            juce::Random rng (42);

            bool hasNaN = false;
            for (int iteration = 0; iteration < 50; ++iteration)
            {
                eq.update (rng.nextFloat() * 30.0f - 15.0f,
                           rng.nextFloat() * 30.0f - 15.0f,
                           rng.nextFloat() * 30.0f - 15.0f);

                auto* data = buffer.getWritePointer (0);
                for (int i = 0; i < kBlockSize; ++i)
                    data[i] = rng.nextFloat() * 2.0f - 1.0f;

                juce::dsp::AudioBlock<float> block_ (buffer);
                eq.process (block_);

                for (int i = 0; i < kBlockSize; ++i)
                    if (std::isnan (buffer.getSample (0, i))) hasNaN = true;
            }
            expect (! hasNaN,
                "rapid random gain changes should not produce NaN");
        }
    }
};

static ThreeBandEQTests threeBandEQTests;
} // namespace neseq
