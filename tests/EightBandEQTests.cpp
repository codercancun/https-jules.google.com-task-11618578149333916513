#include "../source/EightBandEQ.h"

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

/** Feed a sine wave through the given EQ and return the linear RMS of the
    processed signal, measured over the second half of the run. */
float measureRms (EightBandEQ& eq, float freqHz,
                  const std::array<float, EightBandEQ::kNumBands>& gainsDb)
{
    juce::dsp::ProcessSpec spec {};
    spec.sampleRate       = kSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (kBlockSize);
    spec.numChannels      = 1;
    eq.prepare (spec);
    eq.update (gainsDb);

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

float measureGainDb (EightBandEQ& eq, float freqHz,
                     const std::array<float, EightBandEQ::kNumBands>& gainsDb)
{
    return juce::Decibels::gainToDecibels (
        measureRms (eq, freqHz, gainsDb) / kUnitSineRms);
}

std::array<float, EightBandEQ::kNumBands> flat ()
{
    return {};
}

std::array<float, EightBandEQ::kNumBands> withBand (int band, float db)
{
    auto g = flat();
    g[static_cast<size_t> (band)] = db;
    return g;
}
} // namespace

class EightBandEQTests : public juce::UnitTest
{
public:
    EightBandEQTests() : juce::UnitTest ("EightBandEQ") {}

    void runTest() override
    {
        beginTest ("flat response at 0 dB across all band frequencies");
        {
            EightBandEQ eq;
            for (int i = 0; i < EightBandEQ::kNumBands; ++i)
            {
                const auto freq = EightBandEQ::kFrequencies[static_cast<size_t> (i)];
                const auto gainDb = measureGainDb (eq, freq, flat());
                expectWithinAbsoluteError (gainDb, 0.0f, 0.5f,
                    "flat response should be ~0 dB at " + juce::String (freq) + " Hz");
            }
        }

        beginTest ("sub low-shelf boost peaks below 60 Hz");
        {
            EightBandEQ eq;
            auto gains = withBand (0, 12.0f);
            const auto deepLowDb  = measureGainDb (eq, 30.0f,  gains);
            const auto cornerDb   = measureGainDb (eq, 60.0f,  gains);
            const auto farAwayDb  = measureGainDb (eq, 3200.0f, gains);

            expectGreaterThan (deepLowDb, 10.0f,
                "low-shelf @30 Hz with +12 dB should approach full boost");
            expectWithinAbsoluteError (cornerDb, 6.0f, 2.0f,
                "low-shelf @60 Hz corner should boost roughly half the set gain");
            expectWithinAbsoluteError (farAwayDb, 0.0f, 1.5f,
                "low-shelf should barely touch 3.2 kHz");
        }

        beginTest ("high-shelf boost peaks above 12 kHz");
        {
            EightBandEQ eq;
            auto gains = withBand (7, 12.0f);
            const auto deepHighDb = measureGainDb (eq, 20000.0f, gains);
            const auto cornerDb   = measureGainDb (eq, 12000.0f, gains);
            const auto farAwayDb  = measureGainDb (eq, 150.0f,   gains);

            expectGreaterThan (deepHighDb, 10.0f,
                "high-shelf @20 kHz with +12 dB should approach full boost");
            expectWithinAbsoluteError (cornerDb, 6.0f, 2.0f,
                "high-shelf @12 kHz corner should boost roughly half");
            expectWithinAbsoluteError (farAwayDb, 0.0f, 1.5f,
                "high-shelf should barely touch 150 Hz");
        }

        beginTest ("each peak band boosts its centre frequency most");
        {
            for (int band = 1; band <= 6; ++band)
            {
                EightBandEQ eq;
                auto gains = withBand (band, 12.0f);
                const auto centreFreq = EightBandEQ::kFrequencies[static_cast<size_t> (band)];

                const auto atCentre = measureGainDb (eq, centreFreq, gains);
                expectGreaterThan (atCentre, 10.0f,
                    "peak band " + juce::String (band) + " @" +
                    juce::String (centreFreq) + " Hz should boost close to full");

                // Check that a distant frequency is much less affected.
                const float distantFreq = (band <= 3) ? 12000.0f : 60.0f;
                const auto atDistant = measureGainDb (eq, distantFreq, gains);
                expectGreaterThan (atCentre, atDistant + 4.0f,
                    "band " + juce::String (band) + " boost should clearly exceed distant freq");
            }
        }

        beginTest ("cuts attenuate symmetrically with boosts");
        {
            EightBandEQ eq;
            const auto boostDb = measureGainDb (eq, 30.0f, withBand (0, 12.0f));
            const auto cutDb   = measureGainDb (eq, 30.0f, withBand (0, -12.0f));

            expectLessThan (cutDb, -10.0f,
                "low-shelf @30 Hz with -12 dB should cut close to full");
            expectWithinAbsoluteError (boostDb + cutDb, 0.0f, 1.5f,
                "shelf boost and cut should be approximately symmetric");
        }

        beginTest ("coefficient cache: repeated identical updates are no-ops");
        {
            EightBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = kBlockSize;
            spec.numChannels      = 1;
            eq.prepare (spec);

            auto gains = withBand (3, 6.0f);
            eq.update (gains);
            eq.update (gains);
            eq.update (gains);

            juce::AudioBuffer<float> buffer (1, kBlockSize);
            buffer.clear();
            juce::dsp::AudioBlock<float> block_ (buffer);
            juce::dsp::ProcessContextReplacing<float> ctx (block_);
            eq.process (ctx);

            for (int i = 0; i < kBlockSize; ++i)
                expect (std::abs (buffer.getSample (0, i)) < 1.0e-6f,
                    "zero input should produce zero output");
        }

        beginTest ("reset clears filter state to zero");
        {
            EightBandEQ eq;
            juce::dsp::ProcessSpec spec {};
            spec.sampleRate       = kSampleRate;
            spec.maximumBlockSize = kBlockSize;
            spec.numChannels      = 1;
            eq.prepare (spec);
            eq.update (withBand (2, 12.0f));

            // Feed signal.
            juce::AudioBuffer<float> buffer (1, kBlockSize);
            for (int b = 0; b < 8; ++b)
            {
                auto* data = buffer.getWritePointer (0);
                for (int i = 0; i < kBlockSize; ++i)
                    data[i] = static_cast<float> (std::sin (
                        juce::MathConstants<double>::twoPi * 400.0 * (b * kBlockSize + i) / kSampleRate));
                juce::dsp::AudioBlock<float> block_ (buffer);
                juce::dsp::ProcessContextReplacing<float> ctx (block_);
                eq.process (ctx);
            }

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

        beginTest ("multiple bands active simultaneously");
        {
            EightBandEQ eq;
            std::array<float, EightBandEQ::kNumBands> gains {};
            gains[0] = 10.0f;  // sub boost
            gains[7] = -10.0f; // air cut

            const auto lowDb  = measureGainDb (eq, 30.0f,    gains);
            const auto highDb = measureGainDb (eq, 20000.0f, gains);
            const auto midDb  = measureGainDb (eq, 800.0f,   gains);

            expectGreaterThan (lowDb, 8.0f,  "30 Hz should be boosted");
            expectLessThan (highDb, -6.0f,   "20 kHz should be cut");
            expectWithinAbsoluteError (midDb, 0.0f, 3.0f, "800 Hz should be near flat");
        }

        beginTest ("extreme gains ±15 dB");
        {
            EightBandEQ eq;
            const auto maxBoostDb = measureGainDb (eq, 30.0f, withBand (0, 15.0f));
            expectGreaterThan (maxBoostDb, 13.0f,
                "low-shelf @30 Hz with max +15 dB should approach full boost");

            EightBandEQ eq2;
            const auto maxCutDb = measureGainDb (eq2, 30.0f, withBand (0, -15.0f));
            expectLessThan (maxCutDb, -13.0f,
                "low-shelf @30 Hz with max -15 dB should cut heavily");
        }
    }
};

static EightBandEQTests eightBandEQTests;
} // namespace neseq
