#include "../source/PluginProcessor.h"

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <cmath>

namespace neseq
{
namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int    kBlockSize  = 512;
}

class PluginProcessorTests : public juce::UnitTest
{
public:
    PluginProcessorTests() : juce::UnitTest ("PluginProcessor") {}

    void runTest() override
    {
        beginTest ("processBlock passes audio unchanged when bypassed");
        {
            NESEQAudioProcessor processor;
            processor.prepareToPlay (kSampleRate, kBlockSize);

            auto& apvts = processor.getAPVTS();
            if (auto* bypassParam = apvts.getParameter (ParamIDs::bypass))
                bypassParam->setValueNotifyingHost (1.0f); // True for bypass

            juce::AudioBuffer<float> buffer (2, kBlockSize);
            juce::AudioBuffer<float> referenceBuffer (2, kBlockSize);

            for (int ch = 0; ch < 2; ++ch)
            {
                auto* data = buffer.getWritePointer (ch);
                auto* refData = referenceBuffer.getWritePointer (ch);
                for (int i = 0; i < kBlockSize; ++i)
                {
                    // Fill with some dummy data
                    float val = std::sin(i * 0.1f) * (ch + 1) * 0.5f;
                    data[i] = val;
                    refData[i] = val;
                }
            }

            juce::MidiBuffer midiBuffer;
            processor.processBlock (buffer, midiBuffer);

            bool exactlyMatches = true;
            for (int ch = 0; ch < 2; ++ch)
            {
                auto* data = buffer.getReadPointer (ch);
                auto* refData = referenceBuffer.getReadPointer (ch);
                for (int i = 0; i < kBlockSize; ++i)
                {
                    if (data[i] != refData[i])
                    {
                        exactlyMatches = false;
                        break;
                    }
                }
                if (!exactlyMatches) break;
            }

            expect (exactlyMatches, "Output should exactly match input when bypassed");
        }

        beginTest ("processBlock modifies audio when not bypassed");
        {
            NESEQAudioProcessor processor;
            processor.prepareToPlay (kSampleRate, kBlockSize);

            auto& apvts = processor.getAPVTS();
            if (auto* bypassParam = apvts.getParameter (ParamIDs::bypass))
                bypassParam->setValueNotifyingHost (0.0f); // False for bypass

            if (auto* lowParam = apvts.getParameter (ParamIDs::lowGain))
                lowParam->setValueNotifyingHost (1.0f); // Max gain

            juce::AudioBuffer<float> buffer (2, kBlockSize);
            juce::AudioBuffer<float> referenceBuffer (2, kBlockSize);

            for (int ch = 0; ch < 2; ++ch)
            {
                auto* data = buffer.getWritePointer (ch);
                auto* refData = referenceBuffer.getWritePointer (ch);
                for (int i = 0; i < kBlockSize; ++i)
                {
                    // Fill with dummy data
                    float val = std::sin(i * 0.1f) * (ch + 1) * 0.5f;
                    data[i] = val;
                    refData[i] = val;
                }
            }

            juce::MidiBuffer midiBuffer;
            processor.processBlock (buffer, midiBuffer);

            bool exactlyMatches = true;
            for (int ch = 0; ch < 2; ++ch)
            {
                auto* data = buffer.getReadPointer (ch);
                auto* refData = referenceBuffer.getReadPointer (ch);
                for (int i = 0; i < kBlockSize; ++i)
                {
                    if (std::abs(data[i] - refData[i]) > 1e-5f)
                    {
                        exactlyMatches = false;
                        break;
                    }
                }
                if (!exactlyMatches) break;
            }

            expect (!exactlyMatches, "Output should not exactly match input when EQ is applied");
        }
    }
};

static PluginProcessorTests pluginProcessorTests;
} // namespace neseq
