#include "../source/PluginProcessor.h"
#include <juce_core/juce_core.h>

namespace neseq
{
class PluginProcessorTests : public juce::UnitTest
{
public:
    PluginProcessorTests() : juce::UnitTest ("PluginProcessor") {}

    void runTest() override
    {
        beginTest ("Processor instantiates correctly");
        {
            NESEQAudioProcessor processor;

            // Check basic properties
            expectEquals (processor.getName(), juce::String ("NES-EQ"));
            expect (!processor.acceptsMidi());
            expect (!processor.producesMidi());
            expect (!processor.isMidiEffect());
            expectEquals (processor.getTailLengthSeconds(), 0.0);
            expectEquals (processor.getNumPrograms(), 1);
            expectEquals (processor.getCurrentProgram(), 0);

            // Verify createEditor conditionally returns nullptr in test mode
            std::unique_ptr<juce::AudioProcessorEditor> editor { processor.createEditor() };
            expect (editor == nullptr, "createEditor should return nullptr in test mode");
        }

        beginTest ("Processor parameter layout has correct parameters");
        {
            NESEQAudioProcessor processor;
            auto& apvts = processor.getAPVTS();

            auto* lowGain = apvts.getParameter (ParamIDs::lowGain);
            expect (lowGain != nullptr, "Missing low_gain parameter");

            auto* midGain = apvts.getParameter (ParamIDs::midGain);
            expect (midGain != nullptr, "Missing mid_gain parameter");

            auto* highGain = apvts.getParameter (ParamIDs::highGain);
            expect (highGain != nullptr, "Missing high_gain parameter");

            auto* bypass = apvts.getParameter (ParamIDs::bypass);
            expect (bypass != nullptr, "Missing bypass parameter");
        }

        beginTest ("Processor state saving and loading");
        {
            NESEQAudioProcessor processor;
            auto& apvts = processor.getAPVTS();

            // Modify a parameter
            auto* lowGain = apvts.getParameter (ParamIDs::lowGain);
            expect (lowGain != nullptr);
            if (lowGain != nullptr)
            {
                lowGain->setValueNotifyingHost (lowGain->convertTo0to1 (5.0f));
            }

            // Save state
            juce::MemoryBlock stateBlock;
            processor.getStateInformation (stateBlock);
            expect (stateBlock.getSize() > 0, "State block should not be empty");

            // Create a new processor and load state
            NESEQAudioProcessor processor2;
            processor2.setStateInformation (stateBlock.getData(), static_cast<int> (stateBlock.getSize()));

            auto& apvts2 = processor2.getAPVTS();
            auto* lowGain2 = apvts2.getParameter (ParamIDs::lowGain);

            expect (lowGain2 != nullptr);
            if (lowGain != nullptr && lowGain2 != nullptr)
            {
                expectEquals (lowGain2->getValue(), lowGain->getValue(), "Loaded state should match saved state");
            }
        }
    }
};

static PluginProcessorTests pluginProcessorTests;
} // namespace neseq
