#include "../source/PluginProcessor.h"
#include <juce_core/juce_core.h>

namespace neseq
{

// A subclass to expose protected methods or bypass restrictions
class TestableNESEQAudioProcessor : public NESEQAudioProcessor
{
public:
    TestableNESEQAudioProcessor() : NESEQAudioProcessor() {}

    // Override isBusesLayoutSupported to allow any layout so we can test the channel clearing logic
    bool isBusesLayoutSupported([[maybe_unused]] const BusesLayout& layouts) const override
    {
        return true;
    }
};

class PluginProcessorTests : public juce::UnitTest
{
public:
    PluginProcessorTests() : juce::UnitTest ("PluginProcessor") {}

    void runTest() override
    {
        beginTest ("output channels beyond input channels are cleared");
        {
            TestableNESEQAudioProcessor processor;

            // Set up processor with 1 input and 2 output channels
            juce::AudioProcessor::BusesLayout layout;
            layout.inputBuses.add(juce::AudioChannelSet::mono());
            layout.outputBuses.add(juce::AudioChannelSet::stereo());

            processor.setBusesLayout(layout);

            int activeInputs = processor.getTotalNumInputChannels();
            int activeOutputs = processor.getTotalNumOutputChannels();

            processor.setPlayConfigDetails(activeInputs, activeOutputs, 48000.0, 512);
            processor.prepareToPlay(48000.0, 512);

            juce::AudioBuffer<float> buffer(activeOutputs, 512);
            buffer.clear();

            // Fill all channels with garbage
            for (int ch = 0; ch < activeOutputs; ++ch)
            {
                auto* channelData = buffer.getWritePointer(ch);
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                    channelData[i] = 1.0f; // Garbage
            }

            juce::MidiBuffer midi;
            processor.processBlock(buffer, midi);

            // Verify garbage was cleared for channels beyond input
            int expectedCleared = activeOutputs - activeInputs;

            if (expectedCleared > 0)
            {
                for (int ch = activeInputs; ch < activeOutputs; ++ch)
                {
                    auto* processedData = buffer.getReadPointer(ch);
                    int numCleared = 0;
                    for (int i = 0; i < buffer.getNumSamples(); ++i)
                    {
                        if (std::abs(processedData[i]) < 1e-6f)
                            numCleared++;
                    }
                    expectEquals(numCleared, buffer.getNumSamples(), "Output channel " + juce::String(ch) + " should be fully cleared");
                }
            }
            else
            {
                expect(false, "Could not set up test with more outputs than inputs. activeInputs=" + juce::String(activeInputs) + ", activeOutputs=" + juce::String(activeOutputs));
            }

            processor.releaseResources();
        }
    }
};

static PluginProcessorTests pluginProcessorTests;

} // namespace neseq
