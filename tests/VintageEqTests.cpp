#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../VintageEqAudioProcessor.h"

class VintageEqStateTest : public juce::UnitTest
{
public:
    VintageEqStateTest() : juce::UnitTest("VintageEqStateTest") {}

    void runTest() override
    {
        beginTest("State can be saved and restored");

        VintageEqAudioProcessor processor;

        auto* gainParam = processor.treeState.getParameter("GAIN_0");
        gainParam->setValueNotifyingHost(0.8f);
        float expectedGain = gainParam->getValue();

        juce::MemoryBlock stateData;
        processor.getStateInformation(stateData);

        gainParam->setValueNotifyingHost(0.2f);
        expect(gainParam->getValue() != expectedGain);

        processor.setStateInformation(stateData.getData(), (int)stateData.getSize());

        expectEquals(gainParam->getValue(), expectedGain);

        beginTest("Invalid state does not crash");

        char randomData[] = "Not an XML string!";
        processor.setStateInformation(randomData, sizeof(randomData));

        expect(true);
    }
};

static VintageEqStateTest vintageEqStateTest;

int main(int argc, char* argv[])
{
    juce::UnitTestRunner runner;
    runner.runAllTests();

    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (runner.getResult(i)->failures > 0)
            return 1;
    }
    return 0;
}
