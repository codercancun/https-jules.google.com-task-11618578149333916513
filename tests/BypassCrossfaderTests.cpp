#include "../source/BypassCrossfader.h"

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include <cmath>
#include <cstring>

namespace neseq
{
namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int    kBlockSize  = 512;
constexpr int    kNumChannels = 2;
} // namespace

class BypassCrossfaderTests : public juce::UnitTest
{
public:
    BypassCrossfaderTests() : juce::UnitTest ("BypassCrossfader") {}

    void runTest() override
    {
        beginTest ("initial state is fully active");
        {
            BypassCrossfader cf;
            cf.prepare (kSampleRate, kNumChannels, kBlockSize);
            expect (cf.isFullyActive(), "should start fully active");
            expect (! cf.isFullyBypassed(), "should not start bypassed");
            expect (cf.shouldRunEffect(), "should run effect when active");
        }

        beginTest ("fully bypassed copies dry signal");
        {
            BypassCrossfader cf;
            cf.prepare (kSampleRate, kNumChannels, kBlockSize, 0.0);
            cf.setBypassed (true);

            juce::AudioBuffer<float> buffer (kNumChannels, kBlockSize);
            for (int ch = 0; ch < kNumChannels; ++ch)
                for (int i = 0; i < kBlockSize; ++i)
                    buffer.setSample (ch, i, static_cast<float> (ch + 1));

            cf.captureDry (buffer);

            // Simulate wet processing: multiply by 2.
            for (int ch = 0; ch < kNumChannels; ++ch)
                buffer.applyGain (ch, 0, kBlockSize, 2.0f);

            cf.mix (buffer);

            // Output should be the dry signal, not the wet.
            for (int ch = 0; ch < kNumChannels; ++ch)
            {
                for (int i = 0; i < kBlockSize; ++i)
                {
                    expectWithinAbsoluteError (buffer.getSample (ch, i),
                                               static_cast<float> (ch + 1),
                                               1.0e-5f,
                        "bypassed output should match dry input");
                }
            }
        }

        beginTest ("fully active passes wet signal");
        {
            BypassCrossfader cf;
            cf.prepare (kSampleRate, kNumChannels, kBlockSize, 0.0);
            cf.setBypassed (false);

            juce::AudioBuffer<float> buffer (kNumChannels, kBlockSize);
            for (int ch = 0; ch < kNumChannels; ++ch)
                for (int i = 0; i < kBlockSize; ++i)
                    buffer.setSample (ch, i, 1.0f);

            cf.captureDry (buffer);

            // Simulate wet processing.
            for (int ch = 0; ch < kNumChannels; ++ch)
                buffer.applyGain (ch, 0, kBlockSize, 3.0f);

            cf.mix (buffer);

            // Output should be the wet signal (3.0).
            for (int ch = 0; ch < kNumChannels; ++ch)
            {
                for (int i = 0; i < kBlockSize; ++i)
                {
                    expectWithinAbsoluteError (buffer.getSample (ch, i), 3.0f, 1.0e-5f,
                        "active output should match wet signal");
                }
            }
        }

        beginTest ("crossfade ramp produces monotonic transition");
        {
            BypassCrossfader cf;
            const double rampTime = 0.01; // 10 ms
            cf.prepare (kSampleRate, kNumChannels, kBlockSize, rampTime);
            cf.setBypassed (false);

            // Fill with constant dry signal.
            juce::AudioBuffer<float> buffer (1, kBlockSize);
            buffer.clear();
            for (int i = 0; i < kBlockSize; ++i)
                buffer.setSample (0, i, 1.0f);

            cf.captureDry (buffer);

            // Wet = 0.
            buffer.clear();

            // Now request bypass (starts ramping toward dry).
            cf.setBypassed (true);
            expect (cf.isSmoothing() || cf.isFullyBypassed(),
                "should be smoothing or already bypassed after setBypassed(true)");

            cf.mix (buffer);

            // Check that the output ramps upward from 0 toward 1 (dry).
            const auto* data = buffer.getReadPointer (0);
            bool monotonic = true;
            for (int i = 1; i < kBlockSize; ++i)
            {
                if (data[i] < data[i - 1] - 1.0e-6f)
                {
                    monotonic = false;
                    break;
                }
            }
            expect (monotonic, "crossfade ramp should be monotonically increasing");
        }
    }
};

static BypassCrossfaderTests bypassCrossfaderTests;
} // namespace neseq
