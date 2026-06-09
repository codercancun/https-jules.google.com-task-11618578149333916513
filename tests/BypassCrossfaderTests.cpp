#include "../source/BypassCrossfader.h"

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

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

        beginTest ("fully active passes wet signal unmodified");
        {
            BypassCrossfader cf;
            cf.prepare (kSampleRate, 1, kBlockSize);
            cf.setBypassed (false);

            juce::AudioBuffer<float> buffer (1, kBlockSize);
            auto* data = buffer.getWritePointer (0);
            for (int i = 0; i < kBlockSize; ++i)
                data[i] = 1.0f;

            expect (! cf.isFullyBypassed(), "should not be fully bypassed");
            expect (cf.shouldRunEffect(), "should run effect when active");

            cf.mix (buffer);

            for (int i = 0; i < kBlockSize; ++i)
                expectWithinAbsoluteError (data[i], 1.0f, 1.0e-6f,
                    "fully active should not alter the buffer");
        }

        beginTest ("fully bypassed restores dry signal");
        {
            BypassCrossfader cf;
            cf.prepare (kSampleRate, 1, kBlockSize);

            // Run enough blocks to settle the smoother fully to bypassed state
            cf.setBypassed (true);
            for (int b = 0; b < 20; ++b)
            {
                juce::AudioBuffer<float> temp (1, kBlockSize);
                temp.clear();
                if (cf.shouldCaptureDry())
                    cf.captureDry (temp);
                cf.mix (temp);
            }

            expect (cf.isFullyBypassed(), "should be fully bypassed after settling");

            // Now do the real test
            juce::AudioBuffer<float> dryInput (1, kBlockSize);
            auto* dryData = dryInput.getWritePointer (0);
            for (int i = 0; i < kBlockSize; ++i)
                dryData[i] = 0.5f;

            cf.captureDry (dryInput);

            // Simulate wet processing that changes the buffer
            auto* wetData = dryInput.getWritePointer (0);
            for (int i = 0; i < kBlockSize; ++i)
                wetData[i] = 99.0f;

            cf.mix (dryInput);

            for (int i = 0; i < kBlockSize; ++i)
                expectWithinAbsoluteError (dryInput.getSample (0, i), 0.5f, 1.0e-5f,
                    "fully bypassed should restore dry signal");
        }

        beginTest ("crossfade produces intermediate values");
        {
            BypassCrossfader cf;
            cf.prepare (kSampleRate, 1, kBlockSize);
            cf.setBypassed (false);

            // Settle to active state
            for (int b = 0; b < 20; ++b)
            {
                juce::AudioBuffer<float> temp (1, kBlockSize);
                temp.clear();
                cf.mix (temp);
            }

            // Now toggle bypass — during the first block we should see
            // intermediate mixed values (not fully dry or fully wet).
            cf.setBypassed (true);

            juce::AudioBuffer<float> buffer (1, kBlockSize);
            auto* data = buffer.getWritePointer (0);
            for (int i = 0; i < kBlockSize; ++i)
                data[i] = 1.0f; // "wet"

            cf.captureDry (buffer);

            // Make wet different from dry
            for (int i = 0; i < kBlockSize; ++i)
                data[i] = 0.0f; // "processed wet" is 0

            cf.mix (buffer);

            // At least some samples should be between 0 and 1
            bool foundIntermediate = false;
            for (int i = 0; i < kBlockSize; ++i)
            {
                if (data[i] > 0.01f && data[i] < 0.99f)
                {
                    foundIntermediate = true;
                    break;
                }
            }
            expect (foundIntermediate,
                "during crossfade we should see intermediate mixed values");
        }
    }
};

static BypassCrossfaderTests bypassCrossfaderTests;
} // namespace neseq
