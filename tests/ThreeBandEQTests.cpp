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

        beginTest ("shelf boost/cut symmetry");
        {
            ThreeBandEQ eq;
            const float boostDb = measureGainDb (eq, 50.0f,  12.0f, 0.0f, 0.0f);
            const float cutDb   = measureGainDb (eq, 50.0f, -12.0f, 0.0f, 0.0f);

            expectWithinAbsoluteError (boostDb + cutDb, 0.0f, 1.5f,
                "boost and cut should be roughly symmetric around 0 dB");
        }

        beginTest ("coefficient cache: repeated update with unchanged gains is stable");
        {
            ThreeBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = static_cast<juce::uint32> (kBlockSize);
            spec.numChannels      = 1;
            eq.prepare (spec);
            eq.snap (0.0f, 0.0f, 0.0f);

            // Process a block of silence through many identical update cycles.
            juce::AudioBuffer<float> buffer (1, kBlockSize);
            buffer.clear();

            for (int i = 0; i < 50; ++i)
            {
                eq.update (0.0f, 0.0f, 0.0f);
                juce::dsp::AudioBlock<float> block_ (buffer);
                juce::dsp::ProcessContextReplacing<float> ctx (block_);
                eq.process (ctx);
            }

            // Output should still be silence (within floating-point noise).
            float maxAbs = 0.0f;
            const auto* data = buffer.getReadPointer (0);
            for (int i = 0; i < kBlockSize; ++i)
                maxAbs = juce::jmax (maxAbs, std::abs (data[i]));

            expectLessThan (maxAbs, 1.0e-6f,
                "output should remain silent when processing silence with 0 dB gains");
        }

        beginTest ("reset clears filter state");
        {
            ThreeBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = static_cast<juce::uint32> (kBlockSize);
            spec.numChannels      = 1;
            eq.prepare (spec);
            eq.snap (12.0f, 12.0f, 12.0f);

            // Feed a loud sine to build up internal filter state.
            juce::AudioBuffer<float> buffer (1, kBlockSize);
            const double omega = juce::MathConstants<double>::twoPi * 1000.0 / kSampleRate;
            for (int i = 0; i < kBlockSize; ++i)
                buffer.getWritePointer (0)[i] = static_cast<float> (std::sin (omega * i));

            juce::dsp::AudioBlock<float> block_ (buffer);
            juce::dsp::ProcessContextReplacing<float> ctx (block_);
            eq.process (ctx);

            // Reset and process silence.
            eq.reset();
            buffer.clear();

            juce::dsp::AudioBlock<float> block2 (buffer);
            juce::dsp::ProcessContextReplacing<float> ctx2 (block2);
            eq.process (ctx2);

            float maxAbs = 0.0f;
            const auto* data = buffer.getReadPointer (0);
            for (int i = 0; i < kBlockSize; ++i)
                maxAbs = juce::jmax (maxAbs, std::abs (data[i]));

            expectLessThan (maxAbs, 1.0e-4f,
                "output should be near silence after reset + processing silence");
        }

        beginTest ("smoothing: gain ramp eliminates discontinuities");
        {
            ThreeBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = static_cast<juce::uint32> (kBlockSize);
            spec.numChannels      = 1;
            eq.prepare (spec);
            eq.snap (0.0f, 0.0f, 0.0f);

            // Abruptly request +12 dB low boost via update (smoothed).
            eq.update (12.0f, 0.0f, 0.0f);

            // Process a sine tone and check for large inter-sample jumps.
            juce::AudioBuffer<float> buffer (1, kBlockSize);
            const double omega = juce::MathConstants<double>::twoPi * 50.0 / kSampleRate;
            for (int i = 0; i < kBlockSize; ++i)
                buffer.getWritePointer (0)[i] = static_cast<float> (std::sin (omega * i));

            juce::dsp::AudioBlock<float> block_ (buffer);
            juce::dsp::ProcessContextReplacing<float> ctx (block_);
            eq.process (ctx);

            float maxDelta = 0.0f;
            const auto* data = buffer.getReadPointer (0);
            for (int i = 1; i < kBlockSize; ++i)
                maxDelta = juce::jmax (maxDelta, std::abs (data[i] - data[i - 1]));

            // A 50 Hz sine at 48 kHz has a max inter-sample delta of about
            // 2*pi*50/48000 ≈ 0.0065. Even with boost, smoothing should
            // keep it well below a hard discontinuity threshold.
            expectLessThan (maxDelta, 0.5f,
                "smoothed gain change should not produce large discontinuities");
        }
    }
};

static ThreeBandEQTests threeBandEQTests;
} // namespace neseq
