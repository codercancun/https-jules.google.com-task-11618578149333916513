#include "../source/BypassCrossfader.h"

#include <juce_core/juce_core.h>

#include <cmath>

namespace neseq
{
namespace
{
constexpr double kSampleRate   = 48000.0;
constexpr double kRampSeconds  = 0.010;
constexpr int    kRampSamples  = static_cast<int> (kSampleRate * kRampSeconds);
constexpr int    kBlockSize    = 64;

/** Fills ``buffer`` with a value ``dryValue`` on every sample of every
    channel, simulating the raw input. */
void fillDry (juce::AudioBuffer<float>& buffer, float dryValue)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            data[i] = dryValue;
    }
}

/** Fills ``buffer`` with a value ``wetValue`` — stands in for "the effect
    has processed the block in place and produced this wet output". */
void fillWet (juce::AudioBuffer<float>& buffer, float wetValue)
{
    fillDry (buffer, wetValue);
}

void processSettleBlocks (BypassCrossfader& xf,
                          bool  bypassedTarget,
                          int   numBlocks,
                          float dryValue,
                          float wetValue)
{
    juce::AudioBuffer<float> buffer (1, kBlockSize);
    for (int b = 0; b < numBlocks; ++b)
    {
        xf.setBypassed (bypassedTarget);
        fillDry (buffer, dryValue);
        if (xf.shouldCaptureDry())
            xf.captureDry (buffer);

        if (xf.shouldRunEffect())
            fillWet (buffer, wetValue);

        xf.mix (buffer);
    }
}
} // namespace

class BypassCrossfaderTests : public juce::UnitTest
{
public:
    BypassCrossfaderTests() : juce::UnitTest ("BypassCrossfader") {}

    void runTest() override
    {
        beginTest ("fully active passes wet unchanged");
        {
            BypassCrossfader xf;
            xf.prepare (kSampleRate, 1, kBlockSize, kRampSeconds);
            xf.setBypassed (false);

            juce::AudioBuffer<float> buffer (1, kBlockSize);
            fillDry (buffer, 1.0f);
            if (xf.shouldCaptureDry())
                xf.captureDry (buffer);

            // Effect produces a wet value of 2.0.
            fillWet (buffer, 2.0f);
            xf.mix (buffer);

            expect (xf.isFullyActive(), "should start fully active");
            for (int i = 0; i < kBlockSize; ++i)
                expectWithinAbsoluteError (buffer.getSample (0, i), 2.0f, 1.0e-6f,
                    "fully active output should equal wet signal");
        }

        beginTest ("fully bypassed passes dry unchanged once ramp settles");
        {
            BypassCrossfader xf;
            xf.prepare (kSampleRate, 1, kBlockSize, kRampSeconds);
            xf.setBypassed (true);

            // Run enough blocks to finish the ramp.
            const int blocksToSettle = (kRampSamples / kBlockSize) + 4;
            processSettleBlocks (xf, true, blocksToSettle, 1.0f, 2.0f);

            expect (xf.isFullyBypassed(), "should have reached fully-bypassed state");

            // Now the mixer should forward the dry signal unchanged even
            // when the caller skipped wet processing.
            juce::AudioBuffer<float> buffer (1, kBlockSize);
            fillDry (buffer, 3.0f);
            if (xf.shouldCaptureDry())
                xf.captureDry (buffer);

            // Emulate "we skipped running the effect, buffer still holds dry".
            xf.mix (buffer);
            for (int i = 0; i < kBlockSize; ++i)
                expectWithinAbsoluteError (buffer.getSample (0, i), 3.0f, 1.0e-6f,
                    "fully bypassed output should equal dry signal");
        }

        beginTest ("shouldRunEffect becomes false once fully bypassed");
        {
            BypassCrossfader xf;
            xf.prepare (kSampleRate, 1, kBlockSize, kRampSeconds);
            xf.setBypassed (true);

            expect (xf.shouldRunEffect(),
                "effect must still run during the bypass-in ramp so the fade is smooth");

            processSettleBlocks (xf, true, (kRampSamples / kBlockSize) + 4, 1.0f, 2.0f);

            expect (! xf.shouldRunEffect(),
                "after the ramp completes, effect work can be skipped");
        }

        beginTest ("toggle bypass ramps to target within the configured time");
        {
            BypassCrossfader xf;
            xf.prepare (kSampleRate, 1, 1, kRampSeconds);
            xf.setBypassed (true);

            juce::AudioBuffer<float> buffer (1, 1);
            for (int i = 0; i < kRampSamples + 8; ++i)
            {
                fillDry (buffer, 1.0f);
                xf.captureDry (buffer);
                fillWet (buffer, 0.0f);
                xf.mix (buffer);
            }
            expect (xf.isFullyBypassed(),
                "after rampSamples + margin, smoother should be at target");
        }

        beginTest ("crossfade produces no discontinuities on a sine input");
        {
            BypassCrossfader xf;
            xf.prepare (kSampleRate, 1, kBlockSize, kRampSeconds);
            xf.setBypassed (false);

            // Let the smoother settle at fully-active.
            juce::AudioBuffer<float> buffer (1, kBlockSize);

            // Fill with a slow-moving sine and run through while the output
            // equals the input (wet == dry at 0 dB gain). Max sample-to-sample
            // change of a 100 Hz sine at 48 kHz is ~tiny, so any real
            // discontinuity has to come from a glitch in the crossfade itself.
            auto sineAt = [] (double sampleIdx)
            {
                const double freq = 100.0;
                return std::sin (juce::MathConstants<double>::twoPi * freq * sampleIdx / kSampleRate);
            };

            // Prime.
            int globalIdx = 0;
            for (int b = 0; b < 4; ++b)
            {
                for (int i = 0; i < kBlockSize; ++i)
                    buffer.setSample (0, i, (float) sineAt (globalIdx + i));
                xf.captureDry (buffer);
                // wet == dry (effect at unity)
                xf.mix (buffer);
                globalIdx += kBlockSize;
            }

            // Toggle bypass and collect output across the ramp.
            xf.setBypassed (true);

            const int numMonitoredSamples = kRampSamples + kBlockSize * 4;
            std::vector<float> outBuf;
            outBuf.reserve (numMonitoredSamples);

            while ((int) outBuf.size() < numMonitoredSamples)
            {
                for (int i = 0; i < kBlockSize; ++i)
                    buffer.setSample (0, i, (float) sineAt (globalIdx + i));
                xf.captureDry (buffer);
                // wet == dry still
                xf.mix (buffer);
                for (int i = 0; i < kBlockSize; ++i)
                    outBuf.push_back (buffer.getSample (0, i));
                globalIdx += kBlockSize;
            }

            // Largest sample-to-sample delta — with a 100 Hz sine at 48 kHz
            // and wet==dry, the delta per sample stays well under 0.05.
            float maxDelta = 0.0f;
            for (size_t i = 1; i < outBuf.size(); ++i)
                maxDelta = juce::jmax (maxDelta,
                                        std::abs (outBuf[i] - outBuf[i - 1]));

            expectLessThan (maxDelta, 0.05f,
                "crossfade should not introduce abrupt sample-to-sample jumps");
        }
    }
};

static BypassCrossfaderTests bypassCrossfaderTests;
} // namespace neseq
