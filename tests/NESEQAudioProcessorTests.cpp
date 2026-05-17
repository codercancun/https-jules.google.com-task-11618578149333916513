#include "../source/PluginProcessor.h"

#include <juce_core/juce_core.h>

namespace neseq
{
class NESEQAudioProcessorTests : public juce::UnitTest
{
public:
    NESEQAudioProcessorTests() : juce::UnitTest ("NESEQAudioProcessor") {}

    void runTest() override
    {
        beginTest ("State serialization and deserialization");
        {
            NESEQAudioProcessor processor;

            // Set some values
            auto* lowGainParam = processor.getAPVTS().getParameter(ParamIDs::lowGain);
            auto* midGainParam = processor.getAPVTS().getParameter(ParamIDs::midGain);
            auto* bypassParam = processor.getAPVTS().getParameter(ParamIDs::bypass);

            expect(lowGainParam != nullptr, "lowGainParam should not be null");
            expect(midGainParam != nullptr, "midGainParam should not be null");
            expect(bypassParam != nullptr, "bypassParam should not be null");

            if (lowGainParam) lowGainParam->setValueNotifyingHost(lowGainParam->convertTo0to1(12.0f));
            if (midGainParam) midGainParam->setValueNotifyingHost(midGainParam->convertTo0to1(-6.0f));
            if (bypassParam) bypassParam->setValueNotifyingHost(bypassParam->convertTo0to1(true));

            juce::MemoryBlock stateData;
            processor.getStateInformation(stateData);

            // Reset to defaults
            if (lowGainParam) lowGainParam->setValueNotifyingHost(lowGainParam->convertTo0to1(0.0f));
            if (midGainParam) midGainParam->setValueNotifyingHost(midGainParam->convertTo0to1(0.0f));
            if (bypassParam) bypassParam->setValueNotifyingHost(bypassParam->convertTo0to1(false));

            // Restore state
            processor.setStateInformation(stateData.getData(), static_cast<int>(stateData.getSize()));

            // Verify restored values
            if (lowGainParam)
                expect(std::abs(lowGainParam->convertFrom0to1(lowGainParam->getValue()) - 12.0f) < 0.01f, "Low gain should be restored to 12.0f");
            if (midGainParam)
                expect(std::abs(midGainParam->convertFrom0to1(midGainParam->getValue()) - (-6.0f)) < 0.01f, "Mid gain should be restored to -6.0f");
            if (bypassParam)
                expect(std::abs(bypassParam->convertFrom0to1(bypassParam->getValue()) - 1.0f) < 0.01f, "Bypass should be restored to true");
        }
    }
};

static NESEQAudioProcessorTests neseqAudioProcessorTests;
} // namespace neseq
