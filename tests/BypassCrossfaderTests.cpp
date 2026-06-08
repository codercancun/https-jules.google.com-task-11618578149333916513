#include "../source/BypassCrossfader.h"

#include <juce_core/juce_core.h>

#include <cmath>

namespace neseq
{
class BypassCrossfaderTests : public juce::UnitTest
{
public:
    BypassCrossfaderTests() : juce::UnitTest ("BypassCrossfader") {}

    void runTest() override
    {
        constexpr double kSampleRate = 48000.0;
        constexpr int    kBlockSize  = 512;
        constexpr int    kNumCh      = 2;

        beginTest ("dry passthrough when bypassed (no ramp)");
        {
            BypassCrossfader fader;
            fader.prepare (kSampleRate, kNumCh, kBlockSize, 0.015);
            fader.setBypassed (true);

            // Skip past the ramp by processing many blocks.
            for (int block = 0; block < 100; ++block)
            {
                juce::AudioBuffer<float> dry (kNumCh, kBlockSize);
                for (int ch = 0; ch < kNumCh; ++ch)
                    for (int i = 0; i < kBlockSize; ++i)
                        dry.setSample (ch, i, 0.5f);

                juce::AudioBuffer<float> wet (kNumCh, kBlockSize);
                wet.clear(); // wet is silence (EQ output)

                if (fader.shouldCaptureDry())
                    fader.captureDry (dry);

                fader.mix (wet);
            }

            // After ramp, output should be the dry signal.
            juce::AudioBuffer<float> dry (kNumCh, kBlockSize);
            for (int ch = 0; ch < kNumCh; ++ch)
                for (int i = 0; i < kBlockSize; ++i)
                    dry.setSample (ch, i, 0.5f);

            juce::AudioBuffer<float> wet (kNumCh, kBlockSize);
            wet.clear();

            if (fader.shouldCaptureDry())
                fader.captureDry (dry);
            fader.mix (wet);

            for (int ch = 0; ch < kNumCh; ++ch)
                for (int i = 0; i < kBlockSize; ++i)
                    expectWithinAbsoluteError (wet.getSample (ch, i), 0.5f, 1.0e-6f,
                        "bypassed output should equal dry signal");
        }

        beginTest ("wet passthrough when not bypassed");
        {
            BypassCrossfader fader;
            fader.prepare (kSampleRate, kNumCh, kBlockSize, 0.015);
            // Default is not bypassed.

            juce::AudioBuffer<float> wet (kNumCh, kBlockSize);
            for (int ch = 0; ch < kNumCh; ++ch)
                for (int i = 0; i < kBlockSize; ++i)
                    wet.setSample (ch, i, 0.75f);

            fader.mix (wet);

            for (int ch = 0; ch < kNumCh; ++ch)
                for (int i = 0; i < kBlockSize; ++i)
                    expectWithinAbsoluteError (wet.getSample (ch, i), 0.75f, 1.0e-6f,
                        "non-bypassed output should equal wet signal");
        }

        beginTest ("crossfade ramp produces no hard discontinuity");
        {
            BypassCrossfader fader;
            fader.prepare (kSampleRate, kNumCh, kBlockSize, 0.015);

            // Start not bypassed, then toggle to bypass mid-stream.
            float prevSample = -1.0f; // sentinel — set from first output
            bool toggled = false;

            for (int block = 0; block < 10; ++block)
            {
                if (block == 2 && ! toggled)
                {
                    fader.setBypassed (true);
                    toggled = true;
                }

                juce::AudioBuffer<float> buf (kNumCh, kBlockSize);
                for (int ch = 0; ch < kNumCh; ++ch)
                    for (int i = 0; i < kBlockSize; ++i)
                        buf.setSample (ch, i, 1.0f);

                if (fader.shouldCaptureDry())
                    fader.captureDry (buf);

                // Simulate EQ output: processed wet is different from dry.
                if (fader.shouldRunEffect())
                    for (int ch = 0; ch < kNumCh; ++ch)
                        for (int i = 0; i < kBlockSize; ++i)
                            buf.setSample (ch, i, 0.8f);

                fader.mix (buf);

                // Check for continuity: max sample-to-sample jump should be small.
                for (int i = 0; i < kBlockSize; ++i)
                {
                    const float s = buf.getSample (0, i);

                    if (prevSample < 0.0f)
                    {
                        prevSample = s;
                        continue;
                    }

                    const float delta = std::abs (s - prevSample);
                    expect (delta < 0.05f,
                        "sample-to-sample delta should be small during crossfade, got "
                        + juce::String (delta));
                    prevSample = s;
                }
            }
        }

        beginTest ("shouldRunEffect returns false when fully bypassed and settled");
        {
            BypassCrossfader fader;
            fader.prepare (kSampleRate, kNumCh, kBlockSize, 0.015);
            fader.setBypassed (true);

            // Process enough blocks to finish the ramp (~720 samples at 48kHz/15ms).
            for (int block = 0; block < 10; ++block)
            {
                juce::AudioBuffer<float> buf (kNumCh, kBlockSize);
                buf.clear();
                if (fader.shouldCaptureDry())
                    fader.captureDry (buf);
                fader.mix (buf);
            }

            expect (! fader.shouldRunEffect(),
                "shouldRunEffect should be false once bypass ramp is complete");
        }
    }
};

static BypassCrossfaderTests bypassCrossfaderTests;
} // namespace neseq
