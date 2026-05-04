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

class VintageEqProcessBlockTest : public juce::UnitTest
{
public:
    VintageEqProcessBlockTest() : juce::UnitTest("VintageEqProcessBlockTest") {}

    void runTest() override
    {
        beginTest("Processing with valid sample rate modifies buffer");
        {
            VintageEqAudioProcessor processor;
            processor.setPlayConfigDetails(2, 2, 44100.0, 512);
            processor.prepareToPlay(44100.0, 512);

            // Drive band 0 to a non-flat response so the filter is guaranteed to alter the signal.
            // setValueNotifyingHost takes a NORMALISED value in [0, 1].
            processor.treeState.getParameter("GAIN_0")->setValueNotifyingHost(1.0f); // +12 dB
            processor.treeState.getParameter("FREQ_0")->setValueNotifyingHost(0.5f);
            processor.treeState.getParameter("Q_0")->setValueNotifyingHost(0.5f);

            juce::AudioBuffer<float> buffer(2, 512);

            // A constant signal would be a DC component that a peak filter passes unchanged.
            // Use an alternating impulse train to produce broadband content.
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int n = 0; n < buffer.getNumSamples(); ++n)
                    buffer.setSample(ch, n, (n % 2 == 0) ? 1.0f : -1.0f);

            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);

            juce::MidiBuffer midiMessages;
            processor.processBlock(buffer, midiMessages);

            bool changed = false;
            for (int ch = 0; ch < buffer.getNumChannels() && ! changed; ++ch)
                for (int n = 0; n < buffer.getNumSamples(); ++n)
                    if (std::abs(buffer.getSample(ch, n) - original.getSample(ch, n)) > 0.01f)
                    {
                        changed = true;
                        break;
                    }
            expect(changed);
        }

        beginTest("Processing with sample rate <= 0 exits early");
        {
            VintageEqAudioProcessor processor;
            // prepareToPlay has not been called: sample rate must be <= 0 for this test to be meaningful.
            expect(processor.getSampleRate() <= 0.0);

            juce::AudioBuffer<float> buffer(2, 512);
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int n = 0; n < buffer.getNumSamples(); ++n)
                    buffer.setSample(ch, n, 1.0f);

            juce::MidiBuffer midiMessages;
            processor.processBlock(buffer, midiMessages);

            bool unchanged = true;
            for (int ch = 0; ch < buffer.getNumChannels() && unchanged; ++ch)
                for (int n = 0; n < buffer.getNumSamples(); ++n)
                    if (buffer.getSample(ch, n) != 1.0f)
                    {
                        unchanged = false;
                        break;
                    }
            expect(unchanged);
        }

        beginTest("Processing clears extra output channels");
        {
            VintageEqAudioProcessor processor;
            // setPlayConfigDetails bypasses the BusesLayout negotiation and lets us simulate
            // a host providing a buffer with more output channels than input channels.
            processor.setPlayConfigDetails(1, 2, 44100.0, 512);
            processor.prepareToPlay(44100.0, 512);

            juce::AudioBuffer<float> buffer(2, 512);
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                for (int n = 0; n < buffer.getNumSamples(); ++n)
                    buffer.setSample(ch, n, 1.0f);

            juce::MidiBuffer midiMessages;
            processor.processBlock(buffer, midiMessages);

            // The extra output channel (index 1) must be cleared by processBlock.
            bool cleared = true;
            for (int n = 0; n < buffer.getNumSamples(); ++n)
                if (std::abs(buffer.getSample(1, n)) > 0.01f)
                {
                    cleared = false;
                    break;
                }
            expect(cleared);
        }
    }
};

static VintageEqStateTest vintageEqStateTest;
static VintageEqProcessBlockTest vintageEqProcessBlockTest;

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
